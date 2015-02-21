LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/log.c

GLOBAL_DEFINES += \
	HAVE_LOG=1

include make/module.mk