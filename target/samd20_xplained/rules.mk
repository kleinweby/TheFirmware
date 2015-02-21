LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := samd20

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/test_cmd.c

MODULE_DEPS := \
	lib/console

include make/module.mk
