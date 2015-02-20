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
