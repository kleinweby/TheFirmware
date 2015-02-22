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

#include <sensor.h>
#include <malloc.h>

sensor_capabilities_t sensor_get_capabilities(sensor_t sensor)
{
	assert(sensor->ops->get_capabilities != NULL, "get_capabilities is requried");

	return sensor->ops->get_capabilities(sensor);
}

status_t sensor_get_temp(sensor_t sensor, int32_t* result)
{
	if (sensor->ops->get_temp == NULL)
		return STATUS_NOT_SUPPORTED;

	return sensor->ops->get_temp(sensor, result);
}

status_t sensor_get_humidity(sensor_t sensor, int32_t* result)
{
	if (sensor->ops->get_humidity == NULL)
		return STATUS_NOT_SUPPORTED;

	return sensor->ops->get_humidity(sensor, result);
}

status_t sensor_get_voltage(sensor_t sensor, int32_t* result)
{
	if (sensor->ops->get_voltage == NULL)
		return STATUS_NOT_SUPPORTED;

	return sensor->ops->get_voltage(sensor, result);
}

struct {
	list_t list;
} sensors;

void sensors_init()
{
	list_init(&sensors.list);
}

void sensors_register(sensor_t sensor)
{
	list_append(&sensors.list, &sensor->list_entry);
}

void sensors_for_each(bool (*f)(sensor_t sensor, void* context), void* context)
{
	sensor_t sensor;
	list_foreach_contained(sensor, &sensors.list, struct sensor, list_entry) {
		if (!f(sensor, context)) {
			break;
		}
	}
}

struct adc_sensor {
	struct sensor sensor;
	adc_t adc;
	uint32_t mult;
	uint32_t div;
	uint32_t ref;
};

static sensor_capabilities_t sensor_adc_get_capabilities(sensor_t sensor)
{
	return SENSOR_CAPABILITY_VOLTAGE;
}

static status_t sensor_adc_get_voltage(sensor_t _sensor, int32_t* result)
{
	struct adc_sensor* sensor = (struct adc_sensor*)_sensor;

	*result = ((adc_read(sensor->adc) * sensor->mult * sensor->ref) / (sensor->div)) >> adc_resolution(sensor->adc);
	return STATUS_OK;
}

static const struct sensor_ops adc_sensor_ops = {
	.get_capabilities = sensor_adc_get_capabilities,
	.get_voltage = sensor_adc_get_voltage,
};

sensor_t sensor_from_adc(adc_t adc, uint32_t mult, uint32_t div, uint32_t ref)
{
	struct adc_sensor* sensor = malloc(sizeof(struct adc_sensor));

	if (!sensor) {
		return NULL;
	}

	sensor->sensor.ops = &adc_sensor_ops;
	sensor->adc = adc;
	sensor->mult = mult;
	sensor->div = div;
	sensor->ref = ref;

	return (sensor_t)sensor;
}
