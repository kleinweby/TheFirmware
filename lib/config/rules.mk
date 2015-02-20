LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/config.c \
	$(LOCAL_DIR)/config_cmd.c

MODULE_DEPS := \
	lib/crc

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	HAVE_CONFIG=1

include make/module.mk
