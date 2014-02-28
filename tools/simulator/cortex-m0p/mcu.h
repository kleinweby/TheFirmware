//
// Copyright (c) 2014, Christian Speich
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#pragma once

#include <stdint.h>
#include <ev.h>

// Some of these registers are unused
// as these are remapped. We don't care about
// the few ints as it makes the rest way easier
typedef enum {
	// Low registers
	REG_R0   =  0,
	REG_R1   =  1,
	REG_R2   =  2,
	REG_R3   =  3,
	REG_R4   =  4,
	REG_R5   =  5,
	REG_R6   =  6,
	REG_R7   =  7,

	// High registers
	REG_R8   =  8,
	REG_R9   =  9,
	REG_R10  = 10,
	REG_R11  = 11,
	REG_R12  = 12,

	REG_SP   = 13,
	REG_LR   = 14,
	REG_PC   = 15,

	REG_XPSR = 16,

	REG_CONTROL,
	REG_MSP,
	REG_PSP,
	reg_count,
	reg_gdb_count = REG_XPSR + 1,

	REG_APSR = 25,
	REG_IPSR,
	REG_EPSR,
} reg_t;

typedef enum {
	HALT_STOPPED = 0,
	HALT_SLEEP = -1,

	HALT_HARD_FAULT,
	HALT_UNKOWN_INSTRUCTION,

	HAL_TRAP = 5,
} halt_reason_t;

typedef enum {
	exception_reset = 1,
	exception_nmi = 2,
	exception_hardfault = 3,

    exception_svcall = 11,
    exception_pendsv = 14,
    exception_systick = 15,
} exception_t;

typedef enum {
	fault_generic
} fault_t;

typedef int8_t irq_t;

static const irq_t irq_svcall = -5;
static const irq_t irq_pendsv = -2;
static const irq_t irq_systick = -1;

typedef enum {
	processor_thread_mode,
	processor_handler_mode,
} processor_mode_t;

#include_next <mcu.h>

struct mcu_cortex_m0p {
	struct mcu mcu;

	uint32_t regs[reg_count];

	processor_mode_t processor_mode;
};

mcu_t mcu_cortex_m0p_create(struct ev_loop *loop, size_t ramsize);
