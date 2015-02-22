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

#include <arch.h>
#include <platform/irq.h>
#include <bootstrap.h>
#include <runtime.h>
#include <malloc.h>
#include <thread.h>
#include <scheduler.h>
#include <firmware_config.h>
#include <fw/init.h>

#include <stdint.h>

LINKER_SYMBOL(HeapStart, void*);
LINKER_SYMBOL(MainStackTop, void*);

extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

static void arch_handle_irq(void) __attribute__ ((naked, noreturn));
static void arch_handle_pendsv(void) __attribute__ ((naked, noreturn));

static void do_irq_reset(void);

extern void (* const g_pfnVectors[])(void);
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
  &_MainStackTop,
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
  arch_handle_pendsv, // PendSV
  arch_handle_irq, // SysTick

#if ARM_M_NUMBER_OF_IRQS >= 7
  arch_handle_irq, // IRQ0
#endif
#if ARM_M_NUMBER_OF_IRQS >= 8
  arch_handle_irq, // IRQ1
#endif
#if ARM_M_NUMBER_OF_IRQS >= 9
  arch_handle_irq, // IRQ2
#endif
#if ARM_M_NUMBER_OF_IRQS >= 10
  arch_handle_irq, // IRQ3
#endif
#if ARM_M_NUMBER_OF_IRQS >= 11
  arch_handle_irq, // IRQ4
#endif
#if ARM_M_NUMBER_OF_IRQS >= 12
  arch_handle_irq, // IRQ5
#endif
#if ARM_M_NUMBER_OF_IRQS >= 13
  arch_handle_irq, // IRQ6
#endif
#if ARM_M_NUMBER_OF_IRQS >= 14
  arch_handle_irq, // IRQ7
#endif
#if ARM_M_NUMBER_OF_IRQS >= 15
  arch_handle_irq, // IRQ8
#endif
#if ARM_M_NUMBER_OF_IRQS >= 16
  arch_handle_irq, // IRQ9
#endif
#if ARM_M_NUMBER_OF_IRQS >= 17
  arch_handle_irq, // IRQ10
#endif
#if ARM_M_NUMBER_OF_IRQS >= 18
  arch_handle_irq, // IRQ11
#endif
#if ARM_M_NUMBER_OF_IRQS >= 19
  arch_handle_irq, // IRQ12
#endif
#if ARM_M_NUMBER_OF_IRQS >= 20
  arch_handle_irq, // IRQ13
#endif
#if ARM_M_NUMBER_OF_IRQS >= 21
  arch_handle_irq, // IRQ14
#endif
#if ARM_M_NUMBER_OF_IRQS >= 22
  arch_handle_irq, // IRQ15
#endif
#if ARM_M_NUMBER_OF_IRQS >= 23
  arch_handle_irq, // IRQ16
#endif
#if ARM_M_NUMBER_OF_IRQS >= 24
  arch_handle_irq, // IRQ17
#endif
#if ARM_M_NUMBER_OF_IRQS >= 25
  arch_handle_irq, // IRQ18
#endif
#if ARM_M_NUMBER_OF_IRQS >= 26
  arch_handle_irq, // IRQ19
#endif
#if ARM_M_NUMBER_OF_IRQS >= 27
  arch_handle_irq, // IRQ20
#endif
#if ARM_M_NUMBER_OF_IRQS >= 28
  arch_handle_irq, // IRQ21
#endif
#if ARM_M_NUMBER_OF_IRQS >= 29
  arch_handle_irq, // IRQ22
#endif
#if ARM_M_NUMBER_OF_IRQS >= 30
  arch_handle_irq, // IRQ23
#endif
#if ARM_M_NUMBER_OF_IRQS >= 31
  arch_handle_irq, // IRQ24
#endif
#if ARM_M_NUMBER_OF_IRQS >= 32
  arch_handle_irq, // IRQ25
#endif
#if ARM_M_NUMBER_OF_IRQS >= 33
  arch_handle_irq, // IRQ26
#endif
#if ARM_M_NUMBER_OF_IRQS >= 34
  arch_handle_irq, // IRQ27
#endif
#if ARM_M_NUMBER_OF_IRQS >= 35
  arch_handle_irq, // IRQ28
#endif
#if ARM_M_NUMBER_OF_IRQS >= 36
  arch_handle_irq, // IRQ29
#endif
#if ARM_M_NUMBER_OF_IRQS >= 37
  arch_handle_irq, // IRQ30
#endif
#if ARM_M_NUMBER_OF_IRQS >= 38
  arch_handle_irq, // IRQ31
#endif
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

extern void firmware_main();

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

  firmware_main();
}

static uint8_t isr_stack[STACK_SIZE_ISR];
static uint8_t idle_stack[68];
static struct thread idle_thread;
extern void arch_idle_thread(void);

void arch_early_init()
{
  // Setup malloc
  malloc_init((void*)HeapStart, (void*)MainStackTop - STACK_SIZE_MAIN);

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

  // Now create isr stack
  __asm volatile (
    "MSR MSP, %0 \n"
    :
    : "r"(&isr_stack[STACK_SIZE_ISR - 1])
    :
  );
}

__attribute__( ( always_inline ) ) static inline uint32_t __get_PSP(void)
{
  register uint32_t result;

  __asm volatile ("MRS %0, psp\n"  : "=r" (result) );
  return(result);
}

void error()
{
  volatile uint32_t* stack = (uint32_t*)__get_PSP();
  uint32_t eip = stack[-1];
  #pragma unused(eip)
  assert(false, "some error");
}

static void arch_init_idle_thread(fw_init_level_t level)
{
  irq_register(IRQ_HARDFAULT, error);

  thread_struct_init(&idle_thread, "idle", sizeof(idle_stack), (stack_t)idle_stack);
  thread_set_function(&idle_thread, arch_idle_thread, 0);
  scheduler_set_idle_thread(&idle_thread);
}

FW_INIT_HOOK(idle_thread, kFWInitLevelThreading, arch_init_idle_thread);

void arch_init()
{
}

void arch_yield()
{
  *((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV
}

static stack_t arch_dispatch_irq(stack_t stack)
{
  register uint32_t irq;

  __asm volatile ("mrs %0, IPSR\n" : "=r" (irq) );

  // Adjust the irq number, to avoid having many reserved irqs
  if (irq > 11)
    irq -= 7;
  if (irq > 5)
    irq -= 2;

  scheduler_enter_isr();
  do_irq(irq);
  scheduler_leave_isr();

  return stack;
}

static void arch_handle_irq(void)
{
  __asm volatile (
    "PUSH   {lr}                      \n" // Save lr

    //
    // Save state
    //

    // Get saved stack pointer
    "MRS    r3,  PSP                  \n"

    // Make room to save the registers
    "SUBS   r3,  #32                  \n"
    "MOV    r0,  r3                   \n" // r0 is argument

    // Push r4-r7 to the stack
    "STMIA  r3!, {r4-r7}              \n"

    // Push r8-r11
    "MOV    r4,  r8                   \n"
    "MOV    r5,  r9                   \n"
    "MOV    r6,  r10                  \n"
    "MOV    r7,  r11                  \n"
    "STMIA  r3!, {r4-r7}              \n"

    // Call schedule
    "bl     %[dispatch]               \n"

    //
    // Restore state
    //

    // Return stack is in r0

    // Pop r8-r11
    "ADDS    r0,  #16                  \n" // Adjust stack
    "LDMIA   r0!, {r4-r7}              \n"
    "MOV     r8,  r4                   \n"
    "MOV     r9,  r5                   \n"
    "MOV     r10, r6                   \n"
    "MOV     r11, r7                   \n"

    // Pop r4-r7
    "SUBS    r0,  #32                  \n" // Adjust stack
    "LDMIA   r0!, {r4-r7}              \n"

    // Restore stack
    "ADDS    r0,  #16                  \n"

    // Restore stack pointer
    "MSR     PSP, r0                   \n"

    //
    // Exit
    //
    "CPSIE   i                         \n" // Always enable interrupts
    "POP     {pc}                      \n" // Restore lr and return

    :
    : [dispatch]"i"(arch_dispatch_irq)
    :
  );

  // We'll never get here
  unreachable();
}

static void arch_handle_pendsv(void)
{
  __asm volatile (
    "PUSH   {lr}                      \n" // Save lr

    //
    // Save state
    //

    // Get saved stack pointer
    "MRS    r3,  PSP                  \n"

    // Make room to save the registers
    "SUBS   r3,  #32                  \n"
    "MOV    r0,  r3                   \n" // r0 is argument

    // Push r4-r7 to the stack
    "STMIA  r3!, {r4-r7}              \n"

    // Push r8-r11
    "MOV    r4,  r8                   \n"
    "MOV    r5,  r9                   \n"
    "MOV    r6,  r10                  \n"
    "MOV    r7,  r11                  \n"
    "STMIA  r3!, {r4-r7}              \n"

    // Call schedule
    "bl     %[schedule]               \n"

    //
    // Restore state
    //

    // Return stack is in r0

    // Pop r8-r11
    "ADDS    r0,  #16                  \n" // Adjust stack
    "LDMIA   r0!, {r4-r7}              \n"
    "MOV     r8,  r4                   \n"
    "MOV     r9,  r5                   \n"
    "MOV     r10, r6                   \n"
    "MOV     r11, r7                   \n"

    // Pop r4-r7
    "SUBS    r0,  #32                  \n" // Adjust stack
    "LDMIA   r0!, {r4-r7}              \n"

    // Restore stack
    "ADDS    r0,  #16                  \n"

    // Restore stack pointer
    "MSR     PSP, r0                   \n"

    //
    // Exit
    //
    "CPSIE   i                         \n" // Always enable interrupts
    "POP     {pc}                      \n" // Restore lr and return

    :
    : [schedule]"i"(schedule)
    :
  );

  // We'll never get here
  unreachable();
}

typedef struct
{
  volatile uint32_t ISER[1];                 /*!< Offset: 0x000 (R/W)  Interrupt Set Enable Register           */
       uint32_t RESERVED0[31];
  volatile uint32_t ICER[1];                 /*!< Offset: 0x080 (R/W)  Interrupt Clear Enable Register          */
       uint32_t RSERVED1[31];
  volatile uint32_t ISPR[1];                 /*!< Offset: 0x100 (R/W)  Interrupt Set Pending Register           */
       uint32_t RESERVED2[31];
  volatile uint32_t ICPR[1];                 /*!< Offset: 0x180 (R/W)  Interrupt Clear Pending Register         */
       uint32_t RESERVED3[31];
       uint32_t RESERVED4[64];
  volatile uint32_t IPR[8];                  /*!< Offset: 0x3EC (R/W)  Interrupt Priority Register              */
}  NVIC_Type;

#define SCS_BASE            (0xE000E000UL)                            /*!< System Control Space Base Address */
#define NVIC_BASE           (SCS_BASE +  0x0100UL)      
#define NVIC                ((NVIC_Type *)          NVIC_BASE)        /*!< NVIC configuration struct         */

void irq_enable(uint8_t irq_number)
{
  assert(irq_number >= 7, "Can only enable irqs");
  irq_number -= 7;

  NVIC->ISER[0] = (1 << ((uint32_t)(irq_number) & 0x1F));
}

void irq_disable(uint8_t irq_number)
{
  assert(irq_number >= 7, "Can only disable irqs");
  irq_number -= 7;

  NVIC->ICER[0] = (1 << ((uint32_t)(irq_number) & 0x1F));
}
