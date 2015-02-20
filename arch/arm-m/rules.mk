LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/arch_asm.S \
	$(LOCAL_DIR)/systick.c 

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

ifeq ($(ARM_M_NUMBER_OF_IRQS),)
$(error ARM_M_NUMBER_OF_IRQS is not defined)
endif

GLOBAL_DEFINES += \
	ARM_CPU_$(ARM_CPU)=1 \
	ARM_M_NUMBER_OF_IRQS=$(ARM_M_NUMBER_OF_IRQS) \
	NUMBER_OF_IRQS=$(ARM_M_NUMBER_OF_IRQS)

HANDLED_CORE := false
ifeq ($(ARM_CPU),cortex-m0)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M0=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU) -mthumb
THUMBCFLAGS := -mcpu=$(ARM_CPU) -mthumb
HANDLED_CORE := true
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)

ifneq ($(HANDLED_CORE),true)
$(error $(LOCAL_DIR)/rules.mk doesnt have logic for arm core $(ARM_CPU))
endif

include make/module.mk
