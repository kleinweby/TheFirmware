LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := lpc11xx
ENABLE_CAN := 1
XTAL := 16000000UL
# WITH_SHT2X_DEBUG_CMD := 1

HAVE_FAN_OUTPUT ?= 0

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c

MODULE_DEPS := \
	device/24xx64 \
	lib/config \
	lib/can_node

MODULE_DEFINES := \
	HAVE_FAN_OUTPUT=$(HAVE_FAN_OUTPUT)

include make/module.mk
