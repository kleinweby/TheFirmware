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
#include "Firmware/Console/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Time/Systick.h"
#include "LPC11xx.h"

#include <stddef.h>

using namespace TheFirmware::Console;

namespace TheFirmware {
namespace Schedule {

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

///
/// These tasks are ready to run and wait to be scheduled
///
/// These tasks are in this queue mainly because higher
/// priority tasks are currently running 
///
Task* ReadyQueue;

// There is no global waiting queue, as we always have to chain
// waiting task onto the unblocking event and therefore a waiting
// queue would be pointless

Task defaultTask;

// The id for the next new task
TaskID NextTaskID = 0;

// Idle task
Task IdleTask;
uint32_t IdleStack[kMinTaskStackSize];
void Idle();

uint32_t IsrStack[200];

void Init()
{
	(*((volatile uint32_t *)0xE000ED1C)) |=  0xFF000000;
    (*((volatile uint32_t *)0xE000ED20)) |=  0xFFFF0000;

	// Create idle task
	new (&IdleTask) Task(IdleStack, sizeof(IdleStack), Idle, 0);
	IdleTask.setPriority(kIdleTaskPriority);

	//
	// Now we convert our non task to the main task
	//
	new (&defaultTask) Task(NULL, 0, NULL, 0);
	defaultTask.next = &defaultTask; // No other tasks for now,
	defaultTask.state = kTaskStateRunning;
	defaultTask.priority = INT8_MAX; // Avoid loosing control during setup

	CurrentTask = &defaultTask;
	RunningQueue = &defaultTask;

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

	// Now create isr stack
	__asm volatile (
		"MSR MSP, %0 \n"
		:
		: "r"(&IsrStack[199])
		:
	);

	// enable idle task
	IdleTask.setState(kTaskStateReady);
	// Restore normal priority
	defaultTask.setPriority(0);
}

Task* GetCurrentTask()
{
	return CurrentTask;
}

Task::Task(TaskStack stack, size_t stackSize, TaskEntryPoint entryPoint, uint8_t numberOfArgs, ...)
{
	this->next = NULL;
	this->state = kTaskStateWaiting;
	this->priority = 0;
	this->id = NextTaskID++;

	if (stack) {
		assert(stackSize > 0 && entryPoint != NULL, "Specified stack needs entryPoint and size.");
		this->stack = (TaskStack)((uint32_t)stack + stackSize - 4);

		// Prepare stack
		*this->stack-- = (uint32_t)0x01000000L; // PSR
		*this->stack-- = (uint32_t)entryPoint;
		*this->stack   = (uint32_t)0xFFFFFFFEL; // ?
	    this->stack -= 5; // ?

	    if (numberOfArgs > 0) {
	    	va_list args;
	    	va_start(args, numberOfArgs);

	    	// Only get the first argument for now
	    	*this->stack = va_arg(args, uint32_t);

	    	va_end(args);

	    	this->stack -= 8;
	    }
	    else {
	    	this->stack -= 8;
	    }
	}
}

void Task::moveToRunningQueue()
{
	// Remove us from ready queue if we were
	if (this->state == kTaskStateReady) {
		this->removeFromReadyQueue();
	}

	// We're running now
	this->state = kTaskStateRunning;

	// We oust the running tasks
	// Note: check both, as one could be us. (If both are us, we don't care)
	if (this->priority > CurrentTask->getPriority() ||
		this->priority > RunningQueue->getPriority()) {
		// 
		// Every task in running queue has same priority as CurrentTask
		// therefore we will be the only task running now
		//
		this->next = this;

		// Everything in the running queue is a higher priority than
		// any task in ready queue so, just put them in front
		Task* t;
		for (t = RunningQueue; t->next != RunningQueue; t = t->next) {
			// We can set them as ready now
			// Bypass setState here, as we've updates the queues
			t->state = kTaskStateReady;
		}
		t->next = ReadyQueue;
		ReadyQueue = RunningQueue;

		//
		// Now put us as ready queue and inform the system we need
		// a task switch
		//
		RunningQueue = this;
		ForceTaskSwitch();
	}
	else {
		// No we dont oust them, so just prepend us
		Task* t;

		// Append us to the running queue
		// Be aware that running queue is actually a loop
		for (t = RunningQueue; t->next != RunningQueue; t = t->next)
			;
		t->next = this;
		this->next = RunningQueue;
		RunningQueue = this; // Move us to front of queue, is required if running process is alone
		this->state = kTaskStateRunning;
		// No task switch needed
	}
}

void Task::removeFromRunningQueue(bool nextStateReady)
{
	// Remove from running queue
	Task* t;
	for (t = RunningQueue; t->next != this; t = t->next)
		;
	t->next = t->next->next;

	// Insert into ready queue
	if (nextStateReady)
		this->addToReadyQueue();
	else {
		this->state = kTaskStateWaiting;
	}

	// We're currently running
	if (CurrentTask == this) {
		// We're the only process running, we need to rebuild
		// the RunningQueue
		if (RunningQueue == this) {
			RunningQueue = ReadyQueue;

			// Disconnect where the priority drops
			Task* t;
			for (t = RunningQueue; t->next != NULL && RunningQueue->getPriority() == t->next->getPriority(); t = t->next) {
				t->state = kTaskStateRunning;
			}

			t->state = kTaskStateRunning;

			// New head of ready queue
			ReadyQueue = t->next;
			// Close the running queue
			t->next = RunningQueue;
		}
		ForceTaskSwitch();
	}
}

void Task::addToReadyQueue()
{
	if (ReadyQueue) {
		if (this->priority > ReadyQueue->getPriority()) {
			this->next = ReadyQueue;
			ReadyQueue = this;
		}
		else {
			Task* t;
			for (t = ReadyQueue; t->next != NULL && t->next->getPriority() >= this->priority; t = t->next)
				;

			this->next = t->next;
			t->next = this;
		}
	}
	else {
		this->next = NULL;
		ReadyQueue = this;
	}
	
	this->state = kTaskStateReady;
}

void Task::removeFromReadyQueue()
{
	if (this == ReadyQueue) {
		ReadyQueue = ReadyQueue->next;
	}
	else {
		Task* t;
		for (t = ReadyQueue; t->next != NULL && t->next != this; t = t->next)
			;
		t->next = t->next->next;
	}
}

void Task::moveToReadyQueue()
{
	if (this->state == kTaskStateRunning)
		this->removeFromRunningQueue(true);
	else if (this->state == kTaskStateReady) {
		this->removeFromReadyQueue();
		this->addToReadyQueue();
	}
	else
		this->addToReadyQueue();
}

void Task::moveToWaitingQueue()
{
	if (this->state == kTaskStateRunning)
		this->removeFromRunningQueue(false);
	else
		this->removeFromReadyQueue();
}

void Task::setState(TaskState state)
{
	// Don't set state to running here
	// And if we're running dont set it to ready
	if (this->state == state || state == kTaskStateRunning ||
		(this->state == kTaskStateRunning && state == kTaskStateReady))
		return;

	SchedulerLock();
	// We do not actually set the state here, this is done in the move methods

	// We are now blocked, so remove us from scheduling
	if (state == kTaskStateWaiting) {
		// We were running before
		this->moveToWaitingQueue();
	}
	else if (state == kTaskStateReady) {
		// Can we run now?
		if (this->priority >= CurrentTask->getPriority())
			this->moveToRunningQueue();
		else
			this->moveToReadyQueue();
	}
	SchedulerUnlock();
}

void Task::setPriority(TaskPriority priority)
{
	if (this->priority == priority)
		return;

	SchedulerLock();
	TaskPriority oldPriority = this->priority;
	this->priority = priority;

	// When in waiting state, we don't need to adjust
	// the queues
	if (this->state == kTaskStateWaiting) {
		SchedulerUnlock();
		return;
	}

	// We've increased our priority
	if (oldPriority < this->priority) {
		// We're running and oust other running tasks
		// or we now got a priority to allow us running
		if (this->state == kTaskStateRunning ||
			this->priority >= CurrentTask->getPriority()) {

			this->moveToRunningQueue();
		}
		// Still don't have the priority to run,
		// But we're ready so we need to sort the ready queue.
		else if (this->state == kTaskStateReady) {
			this->moveToReadyQueue();
		}
	}
	// We've decreased our priority
	else if (oldPriority > this->priority) {
		// We're running and lost the ability to run
		if (this->state == kTaskStateRunning &&
			(this->priority < CurrentTask->getPriority() || 
			 this->priority < RunningQueue->getPriority() ||
			 this->priority < ReadyQueue->getPriority())) {

			this->moveToReadyQueue();
		}
		// We're ready so just resort the ready queue
		else if (this->state == kTaskStateReady) {
			this->moveToReadyQueue();
		}
	}
	SchedulerUnlock();
}

// Declare Function as naked, to omit default function pro-/epilog
extern "C" void PendSV_Handler(void) __attribute__ ((naked, noreturn));

///
/// Handles the PendSV exception which is used to implement the actual
/// task switch.
///
/// By reprioritiesing this exception to the lowest, it is always tailchained
/// at the end of every irq chain and executes the switch only right before the
/// return to Thread mode.
///
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
		"CPSIE   i                         \n" // Always enable interrupts
    	"BX      lr                        \n"

    	:
		: [CurrentTask]"r" (&CurrentTask), 
		  [RunningQueue]"r" (&RunningQueue),
		  [NextOffset]"n" (__builtin_offsetof(Task, next)),
		  [StackOffset]"n" (__builtin_offsetof(Task, stack))
		:
	);

	// We'll never geht here
	Unreachable();
}

//
// Idle task
//
void Idle()
{
	__asm volatile(
	"loop:      \n"
		"WFI    \n"
		"b loop \n"
	);

	Unreachable();
}

} // namespace Schedule
} // namespace TheFirmware 
