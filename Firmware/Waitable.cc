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

#include "Firmware/Waitable.h"

#include "Firmware/Task.h"

#include <stdarg.h>

namespace TheFirmware {

///
/// Struct that lays on the stack of the waiting tasks
/// and blocks it.
///
/// All methods expect that you hold the SchedulerLock
///
struct Waitee {
	/// The task that waits
	Task::Task* task;
	/// The waitable we're waiting on
	Waitable* waitable;
	/// The next waitee in this queue.
	/// This is NULL if this waitee has been
	/// wooken.
	Waitee* next;

	///
	/// Initializes this waitee und put it in the waiteechain
	/// This does not actually causes the wait but prepares it.
	/// To complete the wait you need to change the tasks state to Waiting
	///
	void init(Waitable* waitable)
	{
		this->waitable = waitable;
		this->task = Task::GetCurrentTask();
		this->next = NULL;

		// Append into waitable chain
		if (this->waitable->waitee) {
			Waitee* last;

			for (last = this->waitable->waitee; last->next != NULL; last = last->next)
				;

			last->next = this;
		}
		// Only waitee
		else 
			this->waitable->waitee = this;

	}

	///
	/// Indicates that his is the waitee actually woken.
	/// (Needed when waiting on multible waitables)
	///
	bool wasWoken()
	{
		// When woken up, task is cleared, so we can check for this
		return this->task == NULL;
	}

	///
	/// Removes the waitee from the chain. This is used when waiting
	/// on multiple waitables and another one woke the task
	///
	void removeEarly()
	{
		// At the beginning of waiting chain
		if (this->waitable->waitee == this)
			this->waitable->waitee = this->next;
		// Further down in chain
		else {
			Waitee* prev;
			// Find prev
			for (prev = this->waitable->waitee; prev->next != this && prev->next != NULL; prev = prev->next)
				;

			// remove us.
			prev->next = this->next;
		}
	}
};

bool Waitable::wakeup()
{
	// No one to wake up
	if (!this->waitee)
		return false;

	// Wakeup the task
	this->waitee->task->setState(Task::kTaskStateReady);

	// Mark that this waitee has been woken
	this->waitee->task = NULL;

	// Advance waitee chain
	this->waitee = this->waitee->next;

	return true;
}

bool Waitable::wakeupAll()
{
	// Wakeup all
	bool hasWokenOne;

	do {
		hasWokenOne = this->wakeup();
	} while(hasWokenOne);

	return hasWokenOne;
}

void Wait(Waitable* waitable)
{
	Waitee waitee;

	Task::SchedulerLock();
	// Waitable does not allow waiting now
	if (!waitable->beginWaiting()) {
		Task::SchedulerUnlock();
		return;
	}
	waitee.init(waitable);
	waitee.task->setState(Task::kTaskStateWaiting); // This releases the scheduler lock, as it will block us
	waitable->endWaiting(false);
}

uint8_t WaitMultiple(uint8_t numberOfWaitables, ...)
{
	Waitee waitees[numberOfWaitables];

	Task::SchedulerLock();
	// Initialize waitees
	va_list args;
	va_start(args, numberOfWaitables);
	for (uint8_t i = 0; i < numberOfWaitables; i++) {
		Waitable* waitable = va_arg(args, Waitable*);
		// Waiting not allowed by this waitable, so clean up
		if (!waitable->beginWaiting()) {
			for (int8_t j = i - 1; j >= 0; j--) {
				waitees[j].removeEarly();
				waitees[j].waitable->endWaiting(true);
			}

			va_end(args);
			Task::SchedulerUnlock();
			return i;
		}
		else {
			waitees[i].init(waitable);
		}
	}
	va_end(args);

	// Wait
	Task::GetCurrentTask()->setState(Task::kTaskStateWaiting);
	// As the task was probbably waiting we most ceartianly lost
	// the lock here. So just reaquire it.
	Task::SchedulerLock();

	// Find out which waitable woke us and remove the others
	uint8_t wokenWaitee = UINT8_MAX;

	for (uint8_t i = 0; i < numberOfWaitables; i++) {
		if (waitees[i].wasWoken()) {
			wokenWaitee = i;
			waitees[i].waitable->endWaiting(false);
		}
		else {
			waitees[i].removeEarly();
			waitees[i].waitable->endWaiting(true);
		}
	}

	Task::SchedulerUnlock();

	return wokenWaitee;
}


} // namespace TheFirmware
