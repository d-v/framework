#pragma once

#include <stdint.h>
#include <modules/platform_stm32f427/platform_stm32f427.h>

#define BOARD_CONFIG_HW_NAME "com.mrobotics.pixracer"
#define BOARD_CONFIG_HW_MAJOR_VER 1
#define BOARD_CONFIG_HW_MINOR_VER 0

#define BOARD_CONFIG_HW_INFO_STRUCTURE                        \
    {                                                         \
        .hw_name = BOARD_CONFIG_HW_NAME,                      \
        .hw_major_version = BOARD_CONFIG_HW_MAJOR_VER,        \
        .hw_minor_version = BOARD_CONFIG_HW_MINOR_VER,        \
        .board_desc_fmt = SHARED_HW_INFO_BOARD_DESC_FMT_NONE, \
        .board_desc = 0,                                      \
    }

#define BOARD_PAL_LINE_CAN_RX PAL_LINE(GPIOD, 0)
#define BOARD_PAL_LINE_CAN_TX PAL_LINE(GPIOD, 1)

#define BOARD_PAL_LINE_UART_TX PAL_LINE(GPIOD, 5)
#define BOARD_PAL_LINE_UART_RX PAL_LINE(GPIOD, 6)

#define BOARD_FLASH_SIZE 2048