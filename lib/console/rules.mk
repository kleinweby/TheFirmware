LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/console.c

GLOBAL_DEFINES += \
	HAVE_CONSOLE=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

include make/module.mk