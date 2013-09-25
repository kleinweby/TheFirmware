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
#include "Firmware/Console/Log.h"
#include "Firmware/Time/Systick.h"
#include "Firmware/Schedule/Task.h"

#include "LPC11xx.h"

using namespace TheFirmware::Console;

namespace TheFirmware {
namespace Time {

namespace {
	struct SysTickConf {
		volatile uint32_t CTRL;
		volatile uint32_t LOAD;
		volatile uint32_t VAL;
		volatile const  uint32_t CALIB;
	};

	enum {
		kCtrlCount       = (1 << 16),
		kCtrlClockSource = (1 << 2),
		kCtrlTickInt     = (1 << 1),
		kCtrlEnable      = (1 << 0),
		kCalibNoRef      = (1 << 31),
		kCalibSkew       = (1 << 30),
		kCalibTenms      = (0xFFFFFF << 0),
	};

	constexpr uint32_t kMaxLoadValue = 0xFFFFFF;

	struct SysTickConf* SysTickConf = (struct SysTickConf*)(0xE000E000UL + 0x0010UL);
}

extern "C" void SysTick_Handler(void);

class SysTickTimer : public Timer {
protected:

	void setFireTime(millitime_t time)
	{
		millitime_t maxPossibleValue = 1000 * kMaxLoadValue/(SystemCoreClock/2);

		if (time > maxPossibleValue)
			time = maxPossibleValue;

		SysTickConf->LOAD = (SystemCoreClock/2)/1000 * time;
		SysTickConf->VAL = 0;
		SysTickConf->CTRL  = kCtrlTickInt | kCtrlEnable;
	}

	millitime_t getFireTime()
	{
		return SysTickConf->LOAD * 1000 / (SystemCoreClock/2);
	}

	millitime_t getRemainingTime()
	{
		return SysTickConf->VAL * 1000 / (SystemCoreClock/2);
	}

	friend void SysTick_Handler();
};

class SysTickTimer _SysTickTimer;
Timer* SysTickTimer = &_SysTickTimer;

extern "C" void SysTick_Handler(void)
{
	_SysTickTimer.isr();
}

} // namespace Time
} // namespace TheFirmware
