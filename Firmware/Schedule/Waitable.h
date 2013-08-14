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

#include <stddef.h>
#include <stdint.h>

namespace TheFirmware {
namespace Schedule {

struct Waitee;
class Waitable;

///
/// Let the current task wait on one waitable
///
/// @param waitable Waitable to wait for
///
void Wait(Waitable* waitable);

///
/// Let the current task wait on multiple waitables
///
/// @param numberOfWaitables Number of waitables to follow
/// @param ... The waitable to wait on
/// @returns the index of the waitable that woke the task
///
uint8_t WaitMultiple(uint8_t numberOfWaitables, ...);

///
/// Waitable is the primitive that is used for tasks
/// to block.
/// It maintains a list of tasks waiting and offers
/// methods to wait, wakeup. To implement waiting
/// there is a Waitee which allows waiting queues and
/// determining which waitable freed up
///
class Waitable {
	/// The waitee chain that is waiting on us.
	Waitee* waitee;

	///
	/// Delegate methods to inform of waiting actions
	/// Those are called with the SchedulerLock in place
	///

	/// 
	/// Inform the waitable that we're about to wait
	/// and give it appropiate chanes to respond to it.
	///
	/// It can also abort the waiting process prematurly
	/// (e.g. a semaphore which has enough resources)
	///
	/// @return wheter the waiting should be continiued (true)
	/// or aborted (false). If aborted WaitMultiple will report
	/// this as success
	///
	virtual bool beginWaiting()
	{
		return true;
	}

	///
	/// Informs the waitable that the wait is over, and
	/// tells wheter the waitee was woken or the wait was
	/// aborted (other waitable woke task)
	///
	/// @param abort True if the waiting on this waitable was
	///              aborted.
	/// @note is not called when beginWaiting returns false
	///
	virtual void endWaiting(bool abort) 
	{
		#pragma unused(abort)
	}

	friend struct Waitee;
	friend void Wait(Waitable* waitable);
	friend uint8_t WaitMultiple(uint8_t numberOfWaitables, ...);
public:
	Waitable() : waitee(NULL) {};

	/// Wakeup one task
	///
	/// @returns True if a task has been woken.
	///
	bool wakeup();

	/// Wakeup all tasks
	///
	/// @returns True if one ore more tasks have been woken.
	///
	bool wakeupAll();
};

} //namespace Schedule
} //namespace TheFirmware
