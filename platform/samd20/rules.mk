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
	$(LOCAL_DIR)/pinmux.c \
	$(LOCAL_DIR)/cmsis/source/system_samd20.c

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)/cmsis/include

GLOBAL_DEFINES += \
	__SAMD20J18__=1 \
	DONT_USE_CMSIS_INIT=1

LINKER_SCRIPT := \
	$(LOCAL_DIR)/link.ld

include make/module.mk