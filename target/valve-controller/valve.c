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

#include "valve.h"

#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <gpio.h>

struct valve {
	bool open;
	int polarity;
	uint32_t open_power;
	uint32_t close_power;
	pin_t pin_a;
	pin_t pin_b;
	int min_voltage;
	int max_voltage;
	adc_t adc;
};

valve_t valve_create(pin_t pin_a, pin_t pin_b, valve_type_t valve_type, adc_t adc)
{
	valve_t valve = malloc_raw(sizeof(struct valve));

	if (!valve) {
		return NULL;
	}

	memset(valve, 0, sizeof(struct valve));

	if (valve_type == VALVE_GARDENA_9V_TYPE) {
		valve->open_power = 20 * 10000;
		valve->close_power = 5.5 * 10000;
		valve->min_voltage = 7000;
		valve->max_voltage = 14000;
	}

	valve->adc = adc;

	gpio_set_direction(valve->pin_a, GPIO_DIRECTION_OUT);
	gpio_set_direction(valve->pin_b, GPIO_DIRECTION_OUT);

	// Ensure that the valve is off
	valve_close(valve);
	valve->polarity = !valve->polarity;
	valve_close(valve);
	valve->polarity = !valve->polarity;

	return valve;
}

bool valve_close(valve_t valve)
{
	uint32_t volt = adc_read(valve->adc);

	if (volt < valve->min_voltage || volt > valve->max_voltage) {
		return false;
	}

	uint32_t duration = valve->close_power / volt;
	int pin = valve->polarity ? valve->pin_a : valve->pin_b;

	gpio_strobe(pin, true, duration);

	return true;
}

bool valve_open(valve_t valve)
{
	uint32_t volt = adc_read(valve->adc);

	if (volt < valve->min_voltage || volt > valve->max_voltage) {
		return false;
	}

	uint32_t duration = valve->open_power / volt;
	int pin = valve->polarity ? valve->pin_a : valve->pin_b;

	gpio_strobe(pin, true, duration);

	return true;
}
