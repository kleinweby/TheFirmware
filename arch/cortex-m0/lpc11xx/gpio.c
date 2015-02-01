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

#include <gpio.h>
#include <scheduler.h>
#include "LPC11xx.h"

static LPC_GPIO_TypeDef (* const LPC_GPIO[4]) = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };

static inline uint8_t pin_get_port(uint32_t pin)
{
	return pin >> 24;
}

static inline uint8_t pin_get_pin(uint32_t pin)
{
	return (pin >> 16) & 0xFF;
}

static inline uint8_t pin_get_number(uint32_t pin)
{
	return pin & 0xFFFF;
}

void gpio_set_direction(pin_t pin, gpio_direction_t direction)
{
	int port = pin_get_port(pin);
	pin = pin_get_pin(pin);

	if (direction == GPIO_DIRECTION_OUT) {
		LPC_GPIO[port]->DIR |= 1<<pin;
	}
	else {
		LPC_GPIO[port]->DIR &= ~(1<<pin);
	}
}

void gpio_set_pull(pin_t pin, gpio_pull_t pull)
{
	volatile uint32_t* conf = NULL;

	size_t offset = pin_get_number(pin);

	if (offset >= 4)
		offset += 4;
	if (offset >= 6 * 4)
		offset += 4;

	conf = OFFSET_PTR(LPC_IOCON, offset);

	switch(pull) {
	case GPIO_PULL_NONE:
		*conf = (*conf & 0xFFFFFFE7);
		break;
	case GPIO_PULL_DOWN:
		*conf = (*conf & 0xFFFFFFE7) | 0x8;
		break;
	case GPIO_PULL_UP:
		*conf = (*conf & 0xFFFFFFE7) | 0x10;
		break;	
	}
}

void gpio_set(pin_t pin, bool on)
{
	int port = pin_get_port(pin);
	pin = pin_get_pin(pin);

	LPC_GPIO[port]->MASKED_ACCESS[(1<<pin)] = on << pin;
}

bool gpio_get(pin_t pin)
{
	int port = pin_get_port(pin);
	pin = pin_get_pin(pin);

	return LPC_GPIO[port]->MASKED_ACCESS[(1<<pin)] >> pin;
}

void gpio_strobe(pin_t pin, bool on, millitime_t time)
{
	gpio_set(pin, on);
	delay(time);
	gpio_set(pin, !on);
}
