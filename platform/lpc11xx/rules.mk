LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm-m
ARM_CPU := cortex-m0
ARM_M_NUMBER_OF_IRQS := 39

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/clock.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/printk.c \
	$(LOCAL_DIR)/i2c.c \
	$(LOCAL_DIR)/CMSIS/src/core_cm0.c \
	$(LOCAL_DIR)/CMSIS/src/system_LPC11xx.c 

ENABLE_CAN ?= 0

ifeq ($(ENABLE_CAN),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/net/can/can_rom.c
GLOBAL_DEFINES += \
	HAVE_CAN=1
endif

MODULE_INCLUDES := \
	$(LOCAL_DIR)/CMSIS/inc

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

LINKER_SCRIPT := \
	$(LOCAL_DIR)/link.ld

include make/module.mk