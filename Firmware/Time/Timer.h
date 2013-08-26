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

#include "Firmware/Schedule/Waitable.h"

#include <stdint.h>

namespace TheFirmware {
namespace Time {

/// Time in microseconds
/// overflows approx. every 24 days
typedef int32_t microtime_t;

class Timeout;

class Timer {
private:

protected:
	Timeout* timeouts;

	/// Set the ms until the timer should fire next
	///
	/// @param time Miliseconds in with the timer should fire
	/// @note the timer may not support the requested time, so
	///       always check getFireTime() to know the actual value
	virtual void setFireTime(microtime_t time) = 0;

	/// Get the miliseconds which the timer was configured to
	virtual microtime_t getFireTime() = 0;

	/// Get remaining time
	virtual microtime_t getRemainingTime() = 0;

	/// Recalculate fire time
	void recalculateFireTime();

	/// Call this from the timer irq
	void isr();

	friend class Timeout;
};

class Timeout {
protected:
	Timeout* next;
	Timer* timer;

	// Remaining time until this timeout fires
	microtime_t remainingTime;

	// Reload time of this timout
	microtime_t timeout;

	struct {
		bool attached;
		bool fired;
		bool repeat;
	};

	// like attach, but does you require to hold the scheduler lock
	void _attach();

	// like detach, but holds scheduler lock
	void _detach();

	// Called by timer when time is up
	// We hold the schedule lock at this point
	void _fire();

	friend class Timer;
public:
	/// Creates and attaches a new timeout to the given timer
	Timeout(microtime_t timeout, Timer* timer, bool repeating = false, bool attach = true);

	~Timeout();

	/// Attaches the timeout to the timer
	void attach();

	/// Detachs the timeout from the timer
	void detach();

	/// Returns wheter the timeout has fired
	bool getHasFired() const
	{
		return this->fired;
	}

	/// Resets the timouts fired flag
	void resetHasFired()
	{
		this->fired = false;
	}

	// Called when the time is up
	virtual void fire() = 0;
};

class WaitableTimeout : public Timeout, public Schedule::Waitable {
private:
	bool beginWaiting();

	void endWaiting(bool abort);

	void fire();

public:
	using Timeout::Timeout;
};

} // namespace Time
} // namespace TheFirmware
