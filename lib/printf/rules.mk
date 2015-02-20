LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/printf.c

GLOBAL_DEFINES += \
	HAVE_PRINTF=1

include make/module.mk