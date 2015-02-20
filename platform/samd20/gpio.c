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

#include <platform/gpio.h>
#include <runtime.h>
#include <stddef.h>

static const void* kGPIORegBase = (void*)0x41004400;

static const uint8_t kGPIORegDirClr = 0x04;
static const uint8_t kGPIORegDirSet = 0x08;
static const uint8_t kGPIORegDataOutput = 0x10;
static const uint8_t kGPIORegDataClr = 0x14;
static const uint8_t kGPIORegDataSet = 0x18;


static ALWAYS_INLINE uint32_t* gpio_reg(pin_t pin, uint8_t reg_offset)
{
	uint8_t port = pin >> 24;

	return OFFSET_PTR(kGPIORegBase, 0x80 * port + reg_offset);
}

static ALWAYS_INLINE uint8_t pin_get_bit(pin_t pin)
{
	return pin & 0x1F;
}

void gpio_set_direction(pin_t pin, gpio_direction_t direction)
{
	if (direction == GPIO_DIRECTION_IN) {
		*gpio_reg(pin, kGPIORegDirClr) = (1 << pin_get_bit(pin));
	}
	else if (direction == GPIO_DIRECTION_OUT) {
		*gpio_reg(pin, kGPIORegDirSet) = (1 << pin_get_bit(pin));
	}
	else {
		assert(false, "Invalid GPIO direction");
	}
}

void gpio_set_pull(pin_t pin, gpio_pull_t pull)
{
	assert(false, "Stub");
}

void gpio_set(pin_t pin, bool on)
{
	if (on) {
		*gpio_reg(pin, kGPIORegDataSet) = (1 << pin_get_bit(pin));
	}
	else {
		*gpio_reg(pin, kGPIORegDataClr) = (1 << pin_get_bit(pin));
	}
}

bool gpio_get(pin_t pin)
{
	assert(false, "Stub");
}

void gpio_strobe(pin_t pin, bool on, millitime_t time)
{
	assert(false, "Stub");
}