BOARD_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
BOARD_SRC = $(BOARD_DIR)/board.c
BOARD_INC = $(BOARD_DIR)
MODULES_ENABLED += platform_stm32f427

USE_LTO = no

USE_OPT = -O0 -g
