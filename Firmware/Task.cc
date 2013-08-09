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

#include "Task.h"
#include "Firmware/Log.h"

using namespace TheFirmware::Log;

namespace TheFirmware {
namespace Task {

//
// The Task currently running
//
Task* CurrentTask;

//
// The queue we're currently running
//
// Tasks in this queue are always the same priority
// and tasks with lower priority that can be run are
// in the ready queue
//
Task* RunningQueue;

Task defaultTask;

Task task2;

uint32_t task2Stack[200];

Task task3;

uint32_t task3Stack[200];

TaskStack InitStack(void(*func)(void*),void *param, TaskStack stack)
{
    *(stack--) = (uint32_t)0x01000000L;      /* xPSR	        */
	*(stack--) = (uint32_t)func;             /* Entry point of task.                         */
	*(stack)   = (uint32_t)0xFFFFFFFEL;
    stack      = stack - 5;
	*(stack)   = (uint32_t)param;            /* r0: argument */
	stack      = stack - 8;
  	
    return (stack);                   /* Returns location of new stack top. */
}

void Blub(void* param)
{
	uint32_t a = (uint32_t)param;

	LogInfo("Start %u", a);

	while (1) {
		LogDebug("Ya");

		*((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV

	}
}

void Blub2(void* param)
{
	uint32_t a = (uint32_t)param;

	LogInfo("Start %u", a);

	while (1) {
		LogWarn("Gna");

		*((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV

	}
}

void Init()
{
	defaultTask.next = &task2;
	defaultTask.stack = (TaskStack)0xA0A0A0A0;

	task2.next = &task3;
	task2.stack = InitStack(Blub, (void*)0xF1, task2Stack + 190);

	task3.next = &defaultTask;
	task3.stack = InitStack(Blub2, (void*)0xAB, task3Stack + 190);

	CurrentTask = &defaultTask;
	RunningQueue = &task2;
}

// Declare Function as naked, to omit default function pro-/epilog
extern "C" void PendSV_Handler(void) __attribute__ ((naked, noreturn));

extern "C" void PendSV_Handler(void)
{
	// Variable we input into an assembler block will be generated
	// before the first instruction we specified.
	//

	// Pointer to CurrentTask is in r0, so do NOT change it, as we're currently have no
	// way of getting it later again. Clang seems to generate this from sp and that we change

	//
	// Load tasks into registers, check if switching needed
	//
	__asm volatile (
		// Get pointers to those to variables.
		// Sadly this will probbably a nop as clang puts
		// those in r0, r1 already but we cannot reley on this :/
	 	"MOVS    r0, %[CurrentTask]        \n" // r0 = &CurrentTask
	 	"MOVS    r1, %[RunningQueue]       \n" // r1 = &RunningQueue

	 	// Get current and next task
		"LDR     r2,  [r0]                 \n" // r2 = CurrentTask (= *r0)
		"LDR     r3,  [r1]                 \n" // r3 = NextTask (= RunningQueue = *r1)

		"CMP     r2,  r3                   \n"
		"BEQ     exitPendSV                \n"

		//
		// Save state
		//

		// Get saved stack pointer
		" MRS    r3,  PSP                  \n" 
  
		// Make room to save the registers
		" SUBS   r3,  #32                  \n"

		// Set task->stack
    	" STR    r3,  [r2, %[StackOffset]] \n"

		// Push r4-r7 to the stack
		" STMIA  r3!, {r4-r7}              \n"

		// Push r8-r11
		" MOV    r4,  r8                   \n"
    	" MOV    r5,  r9                   \n"
    	" MOV    r6,  r10                  \n"
    	" MOV    r7,  r11                  \n"
    	" STMIA  r3!, {r4-r7}              \n" 

		//
		// Update CurrentTask and Running Queue
		//
		"LDR     r2, [r1]                  \n" // r3 = NextTask (= RunningQueue = *r1) (again)
		"STR     r2, [r0]                  \n" // Save NextTask as CurrentTask
		"LDR     r3, [r2, %[NextOffset]]   \n" // Load CurrentTask->next which will become the RunningQueue Head
		"STR     r3, [r1]                  \n" // Save new RunningQueue head

		//
		// Restore state
		//

		"LDR     r3,  [r2, %[StackOffset]] \n" // r3 = NextTask->stack

		// Pop r8-r11
		"ADDS    r3,  #16                  \n" // Adjust stack
    	"LDMIA   r3!, {r4-r7}              \n"
    	"MOV     r8,  r4                   \n"
    	"MOV     r9,  r5                   \n"
    	"MOV     r10, r6                   \n"
    	"MOV     r11, r7                   \n"

    	// Pop r4-r7
    	"SUBS    r3,  #32                  \n" // Adjust stack
    	"LDMIA   r3!, {r4-r7}              \n"

    	// Restore stack
    	"ADDS    r3,  #16                  \n"

    	// Restore stack pointer
    	"MSR     PSP, r3                   \n"

		//
		// Exit
		// 
		"exitPendSV:                       \n"
    	"BX      lr                        \n"

    	:
		: [CurrentTask]"r" (&CurrentTask), 
		  [RunningQueue]"r" (&RunningQueue),
		  [NextOffset]"n" (__builtin_offsetof(Task, next)),
		  [StackOffset]"n" (__builtin_offsetof(Task, stack))
		:
	);

	// We'll never geht here
	__builtin_unreachable();
}

} // namespace Task
} // namespace TheFirmware 
