LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/vfs.c

GLOBAL_DEFINES += \
	HAVE_VFS=1

include make/module.mk