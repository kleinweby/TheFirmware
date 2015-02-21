LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/sht2x.c

MODULE_DEPS := \
	lib/crc \
	lib/sensor

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	HAVE_SHT2X=1

WITH_SHT2X_DEBUG_CMD ?= WITH_DEBUG_CMDS

ifeq ($(WITH_SHT2X_DEBUG_CMD),1)
$(error SHT2X debug command is currently broken)
MODULE_SRCS += \
	$(LOCAL_DIR)/sht2x_cmd.c
endif

include make/module.mk
