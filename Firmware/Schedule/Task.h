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

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace TheFirmware {
namespace Schedule {

/// Pointer to the current task stack pointer
typedef uint32_t* TaskStack;

/// The mimimum stack size needed per task
/// PSR + 16 registers
///
/// @warning a stack smaller than this results in undefined behaiviour
constexpr uint32_t kMinTaskStackSize = 17;

/// Task identifier
typedef uint8_t TaskID;

/// Task priority
typedef int8_t TaskPriority;

/// Function declaration of the task entry point
typedef void(*TaskEntryPoint)();

/// Priority of the idle (WFI) task
/// Is is a task with the lowest priority
/// another task with the same priority is not recommened
static constexpr TaskPriority kIdleTaskPriority = INT8_MIN;

typedef enum : uint8_t {
	/// Unkown state should only happen before the task
	/// is initialized, therfore under any circumstances
	/// this state is encountered, the complete task subsystem
	/// should be considered to be unusable.
	kTaskStateUnkown = 0,

	/// The task is currently running or will be run again soon
	kTaskStateRunning = 1,

	/// The task is ready to be run but wait for higher
	/// priority tasks to finish
	kTaskStateReady = 2, 

	/// The task waits one some resource
	kTaskStateWaiting = 3
} TaskState;

struct Task {
// The following is private, but due to the use of offsetof we canot declare them
// that way

	TaskStack stack;
	TaskID id;
	TaskState state;
	TaskPriority priority;

	/// The next task in chain, depending
	/// on the queue this task is in, it may
	/// be the next task to be executed or simply
	/// another task in a waiting queue
	///
	/// Is NULL when end of list is reached
	Task* next;

	void reschedule();

	// Low Level Helper
	void removeFromRunningQueue(bool nextStateReady);
	void addToReadyQueue();
	void removeFromReadyQueue();

	void moveToRunningQueue();
	void moveToReadyQueue();
	void moveToWaitingQueue();
public:
	// There is no constructor, as constructing task
	// in an undefined order at boot may introduce strange problems
	// use, the init method instead

	///
	/// Initializes the task and put it into waiting state
	///
	/// @param stack the bottom of the stack
	/// @param stackSize size of the stack
	/// @param entryPoint function to start executing
	/// @param numberOfArgs the number of arguments to pass
	/// @param ... arguments to pass
	///            only arguments that can be treated like uint32_t are correctly
	///            passed.
	///
	void Init(TaskStack stack, size_t stackSize, TaskEntryPoint entryPoint, uint8_t numberOfArgs, ...);

	/// Returns the pointer to the current stack pointer
	///
	/// @note This will only be current if it's not the current
	/// task, if you want the stack pointer from the current task
	/// read the corresponding register
	///
	TaskStack getStack() const
	{
		return this->stack;
	}

	/// Returns the id for this task
	TaskID getID() const
	{
		return this->id;
	}

	/// State of the task
	TaskState getState() const
	{
		return this->state;
	}

	/// Changes the state of the task
	///
	/// @praram state the new state
	/// @note You cannot set the state to Running
	///
	void setState(TaskState state);

	/// Priority of the task
	TaskPriority getPriority() const
	{
		return this->priority;
	}

	/// Changes the priority of the task
	///
	/// @param priority Lower numer means higher priority
	///
	void setPriority(TaskPriority priority);
};

///
/// Initialize the Tasks subsystem
///
void Init();

///
/// Returns the currently running task
///
Task* GetCurrentTask();

///
/// Forces a task switch if possible
///
static inline void ForceTaskSwitch()
{
	*((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV
}

///
/// Locks access to the scheduler.
///
/// This mainly disables all irqs as these are the only chance
/// that other scheduling operations could be run.
///
/// @note it does not intervine with ForceTaskSwitch() which will
/// work regardlessly and clear the lock
/// @warning Not a recursive lock
///
static inline void SchedulerLock()
{
	__asm volatile ("cpsid i");
}

///
/// Unlocks the scheduler.
///
/// @see SchedulerLock()
///
static inline void SchedulerUnlock()
{
	__asm volatile ("cpsie i");
}

extern Task defaultTask;

} // namespace Schedule
} // namespace TheFirmware 
