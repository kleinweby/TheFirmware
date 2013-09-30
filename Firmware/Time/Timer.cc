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

#include "Firmware/Time/Timer.h"
#include "Firmware/Schedule/Task.h"
#include "Firmware/Runtime.h"

#include <stddef.h>

namespace TheFirmware {
namespace Time {

void Timer::isr()
{
	Schedule::SchedulerLock();
	if (this->timeouts) {
		this->timeouts->remainingTime -= this->getFireTime();

		if (this->timeouts->remainingTime <= 0) {
			this->timeouts->_fire();
		}
	}

	this->recalculateFireTime();

	Schedule::SchedulerUnlock();
}

void Timer::recalculateFireTime()
{
	if (this->timeouts)
		this->setFireTime(this->timeouts->remainingTime);
}

Timeout::Timeout(millitime_t timeout, Timer* timer, bool repeating, bool attach)
	: next(NULL), timer(timer), remainingTime(0), timeout(timeout), attached(false), fired(false), repeat(repeating)
{
	assert(timeout > 0, "Timeout must be in the futur");
	assert(timer != NULL, "Timer must be specified");

	if (attach)
		this->attach();
}

Timeout::~Timeout()
{
	this->detach();
}

void Timeout::attach()
{
	if (this->attached)
		return;

	Schedule::SchedulerLock();
	this->_attach();
	Schedule::SchedulerUnlock();
}

void Timeout::_attach()
{
	if (this->attached)
		return;

	this->attached = true;

	// There is already a chain, so we insert
	if (this->timer->timeouts) {
		// Need to insert at the beginnging of the queue
		if (this->timer->timeouts->remainingTime > this->timeout) {
			this->next = this->timer->timeouts;
			this->timer->timeouts->remainingTime -= this->timeout;
			this->remainingTime = this->timeout;
			this->timer->timeouts = this;

			this->timer->recalculateFireTime();
		}
		else {
			Timeout* t;
			this->remainingTime = this->timeout;

			for (t = this->timer->timeouts;
				t->next != NULL && t->next->remainingTime > this->remainingTime;
				this->remainingTime -= t->next->remainingTime, t = t->next)
				;

			this->next = t->next;
			t->next = this;
		}
	}
	// Only timeout at the moment
	else {
		this->remainingTime = this->timeout;
		this->next = NULL;
		this->timer->timeouts = this;

		this->timer->recalculateFireTime();
	}	
}

void Timeout::detach()
{
	if (!this->attached)
		return;

	Schedule::SchedulerLock();
	this->_detach();
	Schedule::SchedulerUnlock();
}

void Timeout::_detach()
{
	if (!this->attached)
		return;

	this->attached = false;

	if (this->timer->timeouts == this) {
		this->timer->timeouts = this->next;
	}
	else {
		Timeout* t;

		for (t = this->timer->timeouts;
			 t->next != NULL && t->next != this;
			 t = t->next)
			;

		if (t && t->next) {
			t->next = t->next->next;
		}
	}

	// Reaccount for time
	if (this->next) {
		this->next->remainingTime += this->remainingTime;

		if (this->next->remainingTime < 0) {
			this->next->_fire();
		}
	}

	this->timer->recalculateFireTime();
}

void Timeout::_fire()
{
	this->fired = true;

	this->_detach();
	// Reatach, if needed
	if (this->repeat)
		this->_attach();

	this->fire();
}

// Special case:
//
//   When timeout is 0, we just set fired=true and don't attach to a timer
WaitableTimeout::WaitableTimeout(millitime_t timeout, Timer* timer, bool repeating, bool attach)
	: Timeout(timeout > 0 ? timeout : 1, timer, repeating, timeout > 0 ? attach : false)
{
	if (timeout == 0)
		this->fired = true;
}

bool WaitableTimeout::beginWaiting()
{
	if (this->fired)
		return false;

	return true;
}

void WaitableTimeout::endWaiting(bool abort) 
{
	if (!abort && this->repeat)
		this->resetHasFired();		
}

void WaitableTimeout::fire()
{
	this->wakeupAll();
}

} // namespace Time
} // namespace TheFirmware
