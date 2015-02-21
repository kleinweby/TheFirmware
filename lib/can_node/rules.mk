LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/can_node.c

MODULE_DEPS := \
	lib/sensor \
	lib/config

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	HAVE_CAN_NODE=1

include make/module.mk
