//
// Copyright (c) 2014, Christian Speich
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

#include <runtime.h>
#include <list.h>
#include <adc.h>

typedef ENUM(uint32_t, sensor_capabilities_t) {
	SENSOR_CAPABILITY_TEMP = (1 << 0),
	SENSOR_CAPABILITY_HUMIDITY = (1 << 1),
	SENSOR_CAPABILITY_VOLTAGE = (1 << 2),
};

typedef struct sensor* sensor_t;

struct sensor_ops {
	sensor_capabilities_t (*get_capabilities)(sensor_t sensor);
	// in mili celsius
	status_t (*get_temp)(sensor_t sensor, int32_t* result);
	// in %RH * 1000
	status_t (*get_humidity)(sensor_t sensor, int32_t* result);
	// in micro volt
	status_t (*get_voltage)(sensor_t sensor, int32_t* result);
};

struct sensor {
	list_entry_t list_entry;
	const struct sensor_ops* ops;
};

sensor_capabilities_t sensor_get_capabilities(sensor_t sensor) NONNULL();
status_t sensor_get_temp(sensor_t sensor, int32_t* result) NONNULL();
status_t sensor_get_humidity(sensor_t sensor, int32_t* result) NONNULL();
status_t sensor_get_voltage(sensor_t sensor, int32_t* result) NONNULL();

void sensors_init();
void sensors_register(sensor_t sensor) NONNULL();
void sensors_for_each(bool (*f)(sensor_t sensor, void* context), void* context) NONNULL(1);

// Creates a new sensor from an adc using a suplied resitor devidor
sensor_t sensor_from_adc(adc_t adc, uint32_t mult, uint32_t div, uint32_t ref);
