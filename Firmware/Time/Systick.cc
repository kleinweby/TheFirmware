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

#include "Firmware/Runtime.h"
#include "Firmware/Time/Systick.h"
#include "Firmware/Schedule/Task.h"

#include "LPC11xx.h"

namespace TheFirmware {
namespace Time {

volatile uint32_t CurrentSysTicks = 0;

SysTickTimer* SystickTimerQueue = NULL;

void ArbitTimerQueue()
{
	SystickTimerQueue->remainingMilis -= kMilisPerSysTick;

	while (SystickTimerQueue && SystickTimerQueue->remainingMilis <= 0) {
		SystickTimerQueue->remainingMilis = -1;
		SystickTimerQueue->wakeup();
		SystickTimerQueue = SystickTimerQueue->next;	
	}
}

void EnableSystick()
{
    // 1 Tick = 10ms
    SysTick_Config(SystemCoreClock/100);

	/* Enable the SysTick Counter */
	SysTick->CTRL |= (0x1<<0);
}

void DisableSystick()
{
	Unimplemented();
}

extern "C" void SysTick_Handler(void)
{
	CurrentSysTicks++;

	if (SystickTimerQueue)
		ArbitTimerQueue();

	Schedule::ForceTaskSwitch();
}

SysTickTimer::SysTickTimer(uint32_t milis)
{
	Schedule::SchedulerLock();

	// Queue is empty
	if (!SystickTimerQueue) {
		this->remainingMilis = milis;
		this->next = NULL;
		SystickTimerQueue = this;
	}
	// First timer is after this timer, prepend
	else if (SystickTimerQueue->remainingMilis > milis) {
		SystickTimerQueue->remainingMilis -= milis;
		this->remainingMilis = milis;
		this->next = SystickTimerQueue;
		SystickTimerQueue = this;	
	}
	// Insert timer at appropiated place
	else {
		SysTickTimer* timer;

		for (timer = SystickTimerQueue;
			timer->next != NULL && timer->next->remainingMilis > milis;
			milis -= timer->next->remainingMilis, timer = timer->next)
			;

		this->remainingMilis = milis;
		this->next = timer->next;
		timer->next = this;
	}

	Schedule::SchedulerUnlock();
}

SysTickTimer::~SysTickTimer()
{
	// We only need to remove from the queue when not already fired
	if (this->remainingMilis > 0) {
		Schedule::SchedulerLock();
		if (SystickTimerQueue == this) {
			SystickTimerQueue = SystickTimerQueue->next;
		}
		else {
			SysTickTimer* timer;
			
			for (timer = SystickTimerQueue; timer->next != NULL && timer->next != this; timer = timer->next)
				;

			if (timer)
				timer->next = timer->next->next;
		}
		Schedule::SchedulerUnlock();
	}
}

} // namespace Time
} // namespace TheFirmware
