//
// Copyright (c) 2013, Christian Speich
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

#include <irq.h>
#include <bootstrap.h>

#include <stdint.h>

extern void _vStackTop(void);

extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

#define INT_HANDLER(number) do_irq_##number
#define DECL_INT_HANDLER(number) static void do_irq_##number(void)

DECL_INT_HANDLER(IRQ_NMI);
DECL_INT_HANDLER(IRQ_HARDFAULT);
DECL_INT_HANDLER(IRQ_SV);
DECL_INT_HANDLER(IRQ_PENDSV);
DECL_INT_HANDLER(IRQ_SYSTICK);

DECL_INT_HANDLER(IRQ0);
DECL_INT_HANDLER(IRQ1);
DECL_INT_HANDLER(IRQ2);
DECL_INT_HANDLER(IRQ3);
DECL_INT_HANDLER(IRQ4);
DECL_INT_HANDLER(IRQ5);
DECL_INT_HANDLER(IRQ6);
DECL_INT_HANDLER(IRQ7);
DECL_INT_HANDLER(IRQ8);
DECL_INT_HANDLER(IRQ9);
DECL_INT_HANDLER(IRQ10);
DECL_INT_HANDLER(IRQ11);
DECL_INT_HANDLER(IRQ12);
DECL_INT_HANDLER(IRQ13);
DECL_INT_HANDLER(IRQ14);
DECL_INT_HANDLER(IRQ15);
DECL_INT_HANDLER(IRQ16);

static void do_irq_reset(void);

extern void (* const g_pfnVectors[])(void);
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    &_vStackTop,
    do_irq_reset,
    INT_HANDLER(IRQ_NMI),
    INT_HANDLER(IRQ_HARDFAULT),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    INT_HANDLER(IRQ_SV),
    0,
    0,
    INT_HANDLER(IRQ_PENDSV),
    INT_HANDLER(IRQ_SYSTICK),

    INT_HANDLER(IRQ0),
    INT_HANDLER(IRQ1),
    INT_HANDLER(IRQ2),
    INT_HANDLER(IRQ3),
    INT_HANDLER(IRQ4),
    INT_HANDLER(IRQ5),
    INT_HANDLER(IRQ6),
    INT_HANDLER(IRQ7),
    INT_HANDLER(IRQ8),
    INT_HANDLER(IRQ9),
    INT_HANDLER(IRQ10),
    INT_HANDLER(IRQ11),
    INT_HANDLER(IRQ12),
    INT_HANDLER(IRQ13),
    INT_HANDLER(IRQ14),
    INT_HANDLER(IRQ15),
    INT_HANDLER(IRQ16),
};

static __attribute__ ((section(".after_vectors")))
void data_init(unsigned int romstart, unsigned int start, unsigned int len) {
	unsigned int *pulDest = (unsigned int*) start;
	unsigned int *pulSrc = (unsigned int*) romstart;
	unsigned int loop;
	for (loop = 0; loop < len; loop = loop + 4)
		*pulDest++ = *pulSrc++;
}

static __attribute__ ((section(".after_vectors")))
void bss_init(unsigned int start, unsigned int len) {
	unsigned int *pulDest = (unsigned int*) start;
	unsigned int loop;
	for (loop = 0; loop < len; loop = loop + 4)
		*pulDest++ = 0;
}

static void do_irq_reset(void)
{
	//
    // Copy the data sections from flash to SRAM.
    //
	unsigned int LoadAddr, ExeAddr, SectionLen;
	unsigned int *SectionTableAddr;

	// Load base address of Global Section Table
	SectionTableAddr = &__data_section_table;

    // Copy the data sections from flash to SRAM.
	while (SectionTableAddr < &__data_section_table_end) {
		LoadAddr = *SectionTableAddr++;
		ExeAddr = *SectionTableAddr++;
		SectionLen = *SectionTableAddr++;
		data_init(LoadAddr, ExeAddr, SectionLen);
	}
	// At this point, SectionTableAddr = &__bss_section_table;
	// Zero fill the bss segment
	while (SectionTableAddr < &__bss_section_table_end) {
		ExeAddr = *SectionTableAddr++;
		SectionLen = *SectionTableAddr++;
		bss_init(ExeAddr, SectionLen);
	}

	bootstrap();
}

void arch_early_init()
{
	// UART is already possible
}

void arch_late_init()
{
	// Now we switch the used stack from msp to psp
    __asm volatile (
        // Move to psp
        "MRS R1, MSP\n"
        "MSR PSP, R1\n"
        "MOVS R1, #2\n"
        "MSR CONTROL, R1\n"
        :
        :
        : "r1"
    );

    // // Now create isr stack
    // __asm volatile (
    //     "MSR MSP, %0 \n"
    //     :
    //     : "r"(&IsrStack[199])
    //     :
    // );
}

#define DEF_INT_HANDLER(number) static void do_irq_##number(void) { \
	\
}

DEF_INT_HANDLER(IRQ_NMI);
DEF_INT_HANDLER(IRQ_HARDFAULT);
DEF_INT_HANDLER(IRQ_SV);
DEF_INT_HANDLER(IRQ_PENDSV);
DEF_INT_HANDLER(IRQ_SYSTICK);

DEF_INT_HANDLER(IRQ0);
DEF_INT_HANDLER(IRQ1);
DEF_INT_HANDLER(IRQ2);
DEF_INT_HANDLER(IRQ3);
DEF_INT_HANDLER(IRQ4);
DEF_INT_HANDLER(IRQ5);
DEF_INT_HANDLER(IRQ6);
DEF_INT_HANDLER(IRQ7);
DEF_INT_HANDLER(IRQ8);
DEF_INT_HANDLER(IRQ9);
DEF_INT_HANDLER(IRQ10);
DEF_INT_HANDLER(IRQ11);
DEF_INT_HANDLER(IRQ12);
DEF_INT_HANDLER(IRQ13);
DEF_INT_HANDLER(IRQ14);
DEF_INT_HANDLER(IRQ15);
DEF_INT_HANDLER(IRQ16);
