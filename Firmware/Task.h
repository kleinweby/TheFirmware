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

namespace TheFirmware {

/// Pointer to the current task stack pointer
/// The minimum task size is 16*4 (for r0-r15 32bit wide registers)
typedef uint32_t* TaskStack;
/// Task identifier
typedef uint8_t TaskID;

/// Task priority
/// Lower number means _higher_ priority
/// (As in unix)
typedef int8_t TaskPriority;

/// Priority of the idle (WFI) task
/// Is is a task with the lowest priority
/// another task with the same priority is not recommened
static constexpr TaskPriority kIdleTaskPriority = INT8_MAX;

typedef enum : uint8_t {
	/// Unkown state should only happen before the task
	/// is initialized, therfore under any circumstances
	/// this state is encountered, the complete task subsystem
	/// should be considered to be unusable.
	kTaskStateUnkown = 0,

	/// The task is ready to be run and will be run soon
	///
	/// @note the current task is also always ready
	kTaskStateReady = 1, 

	/// The task waits one some resource
	kTaskStateWaiting = 2
} TaskState;

class Task {
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
	//Task* next;

public:

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

	/// Priority of the task
	TaskPriority getPriority() const
	{
		return this->priority;
	}

	/// Changes the priority of the task
	///
	/// @param priority Lower numer means higher priority
	///
	void setPriority(TaskPriority priority)
	{
		this->priority = priority;

		/// TODO: need to inform scheduler here and rebuild queues
	}
};



} // namespace TheFirmware 
