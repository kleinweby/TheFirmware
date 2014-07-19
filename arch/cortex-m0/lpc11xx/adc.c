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

#include <adc.h>
#include <malloc.h>

#include "LPC11xx.h"

struct adc {
	pin_t pin;
};

adc_t adc_create(pin_t pin)
{
	adc_t adc = malloc_raw(sizeof(struct adc));

	if (!adc) {
		return NULL;
	}

	// Disable Power down bit to the ADC block.
	LPC_SYSCON->PDRUNCFG &= ~(0x1<<4);
	// Enable ADC
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<13);
	// Set clock
	LPC_ADC->CR = ((SystemCoreClock/LPC_SYSCON->SYSAHBCLKDIV)/4500000-1)<<8;
	LPC_IOCON->PIO1_11   = 0x01;

	return adc;
}

uint32_t adc_read(adc_t adc)
{
	static const uint8_t chan = 7;

	// Start conv
	LPC_ADC->CR &= 0xFFFFFF00;
	LPC_ADC->CR |= (1 << 24) | (1 << chan);

	uint32_t val;

	do {
		val = LPC_ADC->GDR;
	} while (!(val & (1 << 31))); // Done bit

	// Stop
	LPC_ADC->CR &= 0xF8FFFFFF;

	int human_val = (((val >> 6) & 0x3FF) * 16) * 3300;

	return human_val / 1000;
}
