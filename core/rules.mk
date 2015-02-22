LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/list.c \
	$(LOCAL_DIR)/malloc.c \
	$(LOCAL_DIR)/runtime.c \
	$(LOCAL_DIR)/scheduler.c \
	$(LOCAL_DIR)/semaphore.c \
	$(LOCAL_DIR)/string.c \
	$(LOCAL_DIR)/test.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/file.c 

include make/module.mk