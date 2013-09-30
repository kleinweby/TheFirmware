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

#include "Firmware/Schedule/Semaphore.h"
#include "Firmware/Time/Systick.h"

#include <stdint.h>

namespace TheFirmware {
namespace Utils {

template<size_t size>
class Ringbuffer {
private:
	size_t getIndex;
	Schedule::Semaphore getSemaphore;
	size_t putIndex;
	Schedule::Semaphore putSemaphore;
	char buffer[size];

public:
	Ringbuffer() : getIndex(0), getSemaphore(0), putIndex(0), putSemaphore(size)
	{}

	bool isEmpty()
	{
		return this->putIndex == this->getIndex;
	}

	bool canGet()
	{
		return this->putIndex != this->getIndex;
	}

	bool canPut()
	{
		return (this->putIndex + 1)%size != this->getIndex;
	}

	bool get(char& c, Time::millitime_t timeout = -1)
	{
		if (timeout == 0 && !this->canGet())
			return false;
		// Wait forever
		else if (timeout < 0) {
			this->getSemaphore.wait();
		}
		else {
			Time::WaitableTimeout timer(timeout, Time::SysTickTimer);

			if (Schedule::WaitMultiple(&this->getSemaphore, &timer) == 1) {
				return false;
			}
		}

		c = this->buffer[this->getIndex];
		this->getIndex = (this->getIndex + 1) % size;
		this->putSemaphore.signal();

		return true;
	}

	bool put(char c, bool force = false, Time::millitime_t timeout = -1)
	{
		bool overflow = false;

		if (force)
			timeout = 0;

		if (timeout == 0 && !this->canPut()) {
			if (force)
				overflow = true;
			else
				return false;
		}
		// Wait forever
		else if (timeout < 0) {
			this->putSemaphore.wait();
		}
		else {
			Time::WaitableTimeout timer(timeout, Time::SysTickTimer);

			if (Schedule::WaitMultiple(&this->putSemaphore, &timer) == 1) {
				return false;
			}
		}

		this->buffer[this->putIndex] = c;
		this->putIndex = (this->putIndex + 1) % size;
		if (overflow)
			// Buffer is full, so drop the oldes byte
			this->getIndex = (this->getIndex + 1) % size;
		else
			this->getSemaphore.signal();

		return true;
	}
};

} // namespace Utils
} // namespace Ringbuffer
