LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	core \
	platform \
	target \
	lib/printf \
	lib/log \
	lib/console \
	lib/vfs

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/main.c

MODULE_LDFLAGS := -lgcc

include make/module.mk