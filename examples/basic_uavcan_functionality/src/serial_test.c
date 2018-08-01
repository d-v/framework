#include "framework_conf.h"
#include <modules/worker_thread/worker_thread.h>
#include "hal.h"
#include "chconf.h"
#include <stdarg.h> //for va_list
#include <memstreams.h> //for MemStream
#include <chprintf.h>
#include <modules/uavcan/uavcan.h>
#include <common/helpers.h> //MIN
#include <modules/uavcan_debug/uavcan_debug.h>
#include <uavcan.tunnel.Broadcast.h>

#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)

#define USART_CR1_9BIT_WORD (1 << 12)  /* CR1 9 bit word */
#define USART_CR1_PARITY_SET (1 << 10) /* CR1 parity bit enable */
#define USART_CR1_EVEN_PARITY (0 << 9) /* CR1 even parity */

struct worker_thread_publisher_task_s publisher_task; //idle event publisher
static struct pubsub_topic_s rx_irq_topic; //the topic for the idle event

void rx_irq_cb(void* self){ //callback for idle
    volatile uint16_t sr = ((SerialDriver *)(self))->usart->SR;
    if (sr & USART_SR_IDLE)
    {
        volatile uint16_t dr = ((SerialDriver *)(self))->usart->DR;
        (void) dr;
        chSysLockFromISR(); //mutex lock
        worker_thread_publisher_task_publish_I(&publisher_task, //create the publisher
                            &rx_irq_topic, sizeof(dr), pubsub_copy_writer_func, &dr);
        chSysUnlockFromISR(); //mutex unlock
    }
}

static SerialConfig sd2cfg = {
    115200,
    USART_CR1_IDLEIE, //enable the idle interrupts
    0,
    0,
    rx_irq_cb,
    ((void*)&SD2)};

static void SendString(SerialDriver *sdp, const char *string);

static struct worker_thread_timer_task_s serial_test_task;
static void serial_test_task_func(struct worker_thread_timer_task_s *task);

static struct worker_thread_listener_task_s tunnel_listener_task;
static void tunnel_handler(size_t msg_size, const void* buf, void* ctx);

#define UAVCAN_TUNNEL_PROTOCOL_GPS 5
#define CANARD_TRANSFER_PRIORITY_LOWEST 31

static void SendString(SerialDriver *sdp, const char *string)
{
    uint8_t i;
    for (i = 0; string[i] != '\0'; i++)
        sdPut(sdp, string[i]);
}

RUN_AFTER(INIT_END)
{
    struct pubsub_topic_s* tunnel_topic = uavcan_get_message_topic(0, &uavcan_tunnel_Broadcast_descriptor);
    worker_thread_add_listener_task(&WT, &tunnel_listener_task, tunnel_topic, tunnel_handler, NULL); //add tunnel listener

    worker_thread_add_publisher_task(&WT, &publisher_task, sizeof(uint16_t), 16); // add the publisher callback
    pubsub_init_topic(&rx_irq_topic, NULL); //create the publisher subscriber topic

    static struct worker_thread_listener_task_s idle_listener;
    worker_thread_add_listener_task(&WT,
        &idle_listener, &rx_irq_topic, serial_test_task_func, NULL); //start the idle listener task
    
    sdStart(&SD2, &sd2cfg); //Start serial driver
}

static void tunnel_handler(size_t msg_size, const void* buf, void* ctx) {

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_tunnel_Broadcast_s* msg = (const struct uavcan_tunnel_Broadcast_s*)msg_wrapper->msg;

    sdWrite(&SD2, (uint8_t *)msg->buffer, msg->buffer_len);
}

void uavcan_send_tunnel_msg(const char *buf, uint16_t buflen){
    struct uavcan_tunnel_Broadcast_s tunnel_broadcast;
    memcpy(tunnel_broadcast.buffer, buf, buflen);
    tunnel_broadcast.buffer_len = buflen;
    tunnel_broadcast.protocol.protocol = UAVCAN_TUNNEL_PROTOCOL_GPS;
    uavcan_broadcast(0, &uavcan_tunnel_Broadcast_descriptor, CANARD_TRANSFER_PRIORITY_LOWEST, &tunnel_broadcast);
}

static void serial_test_task_func(struct worker_thread_timer_task_s *task)
{
    msg_t charbuf;
    
    uint16_t i = 0;
    do {
        char bufchar[60];
        for(i = 0; i < 60; i++)
        {
            charbuf = chnGetTimeout(&SD2, TIME_IMMEDIATE);
            if (charbuf != Q_TIMEOUT)
            {
                bufchar[i] = charbuf; //populate the string with chars
            }
            else
            {
                break; // there are no remaining chars
            }
        }
        if (i > 0)
        {
            uavcan_send_tunnel_msg(bufchar, i); //send the message
        }
    } while (charbuf != Q_TIMEOUT);
}