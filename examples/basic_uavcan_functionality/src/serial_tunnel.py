import uavcan
import threading
import serial
import io
import argparse

parser = argparse.ArgumentParser(description='Send and receive text over serial and uavcan.tunnel.Broadcast')
parser.add_argument('--uavcan', required=True)
parser.add_argument('--serial', required=True)
args = parser.parse_args()

print('Starting UAVCAN listener on ', args.uavcan)
print('Starting serial listener on ', args.serial)

node_info = uavcan.protocol.GetNodeInfo.Response()
node_info.name = 'org.uavcan.serial_tunnel_client'

node = uavcan.make_node(args.uavcan, node_id=123, node_info=node_info)  # Setting node info
                        
threading.Thread(target=node.spin).start()

ser = serial.Serial(args.serial,115200, timeout=None)

alive = False

def tunnel_callback(event):
    print("received uavcan.tunnel.Broadcast message:")
    print(uavcan.to_yaml(event))

handle = node.add_handler(uavcan.tunnel.Broadcast, tunnel_callback)


def receiveserialdata():
    global alive
    global ser
    while alive:
        text = ser.read(1)
        if text:
            n = ser.inWaiting()
            if n:
                text = text + ser.read(n)
            alive = False
        print("received on serial: "+text.decode("utf-8"))

def senddata():
    while True:
        try:
            message = input()
            first = message.split(' ', 1)[0]
            rest  = message.split(' ', 1)[1]
            if first == 'uavcan':
                do_uavcan_publish(rest)
            elif first == 'serial':
                do_serial_publish(rest)
            else:
                print('Please prepend the message with either uavcan or serial')
        except UAVCANException as ex:
            print('Node error:', ex)
        
def do_uavcan_publish(str):
    global alive
    print("publishing uavcan.tunnel.Broadcast")
    msg = uavcan.tunnel.Broadcast(buffer=str)
    alive = True
    node.broadcast(msg, priority=uavcan.TRANSFER_PRIORITY_HIGHEST)
    receiveserialdata()

def do_serial_publish(string):
    print("publishing serial data: ", string)
    ser.write(str.encode(string))

senddata()
