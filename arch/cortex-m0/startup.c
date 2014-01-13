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
#include <runtime.h>

#include <stdint.h>

extern void _vStackTop(void);

extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

static void arch_handle_irq(void);

static void do_irq_reset(void);

extern void (* const g_pfnVectors[])(void);
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    &_vStackTop,
    do_irq_reset,
    arch_handle_irq, // NMI
    arch_handle_irq, // HardFault
    0,
    0,
    0,
    0,
    0,
    0,
    0,
   	arch_handle_irq, // SVCall
    0,
    0,
    arch_handle_irq, // PendSV
    arch_handle_irq, // SysTick

    arch_handle_irq, // IRQ0
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
    arch_handle_irq,
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
        "mrs r1, MSP\n"
        "msr PSP, r1\n"
        "movs r1, #2\n"
        "msr CONTROL, r1\n"
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

static void arch_handle_irq(void)
{
	register uint32_t irq;

  	__asm volatile ("mrs %0, IPSR\n" : "=r" (irq) );

  	do_irq(irq);
}
