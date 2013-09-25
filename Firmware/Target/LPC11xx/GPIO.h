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

#include "Firmware/GPIO.h"
#include "LPC11xx.h"

namespace TheFirmware {
namespace LPC11xx {

static LPC_GPIO_TypeDef (* const LPC_GPIO[4]) = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };

class GPIO {
	int port;
	int bit;
public:
	GPIO(int port, int bit) : port(port), bit(bit)
	{
	}

	GPIODirection getDirection() const 
	{
		switch((LPC_GPIO[port]->DIR > bit) & 0x1) {
			case 0:
				return GPIODirectionInput;
			case 1:
				return GPIODirectionOutput;
			default:
				return GPIODirectionUnkown;
		}
	}

	void setDirection(GPIODirection dir) 
	{
		switch (dir) {
			case GPIODirectionOutput:
				LPC_GPIO[port]->DIR |= 1<<bit;
				break;
			case GPIODirectionInput:
				LPC_GPIO[port]->DIR &= ~(1<<bit);
				break;
			default:
				break;
		}
	}

	bool get() {
		return LPC_GPIO[port]->MASKED_ACCESS[(1<<bit)];
	}

	void set(bool on)
	{
		LPC_GPIO[port]->MASKED_ACCESS[(1<<bit)] = on << bit;
	}
};

} // namespace LPC11xx
} // namespace TheFirmware
