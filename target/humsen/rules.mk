LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := lpc11xx
ENABLE_CAN := 1
XTAL := 16000000UL
# WITH_SHT2X_DEBUG_CMD := 1

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c

MODULE_DEPS := \
	device/24xx64 \
	device/sht2x \
	lib/config \
	lib/can_node

include make/module.mk
