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

#include "can_node.h"

#include <string.h>
#include <config/config.h>
#include <scheduler.h>
#include <semaphore.h>
#include <sensor.h>
#include <log.h>
#include <platform/gpio.h>

bool can_node_valid_id(can_node_id_t id)
{
	return (id & 0xFC00) == 0;	
}

#define assert_can_node_id(id) assert(can_node_valid_id(id), "Can node id can only be 10 bit.")

typedef ENUM(uint32_t, can_node_work_req_t) {
	kCanNodeWorkReqSensors = (1 << 0),
	kCanNodeWorkReqOutput = (1 << 1),
};

struct _output_src {
	bool state;
};

struct _output {
	bool state;
	pin_t pin;
	struct _output_src srcs[CAN_NODE_NUM_OUTPUT_SOURCE];
};

struct _can_node {
	can_node_id_t node_id;
	struct semaphore sem;
	can_node_work_req_t reqs;
	struct _output outputs[CAN_NODE_NUM_OUTPUT];
};

static struct _can_node can_node;

static can_node_work_req_t can_node_fetch_clear_reqs()
{
	scheduler_lock();
	can_node_work_req_t reqs = can_node.reqs;
	can_node.reqs = 0;
	scheduler_unlock();

	return reqs;
}

static can_id_t can_id_build(can_node_id_t from, can_node_id_t to, can_node_topic_t topic)
{
	return ((topic & 0x1FF) << 20) | ((from & 0x3FF) << 10) | (to & 0x3FF);
}

static can_node_id_t can_id_extract_to(can_id_t id)
{
	return id & 0x3FF;
}

static can_node_id_t can_id_extract_from(can_id_t id)
{
	return (id >> 10) & 0x3FF;
}

static can_node_topic_t can_id_extract_topic(can_id_t id)
{
	return (id >> 20) & 0x1FF;
}

static void can_node_send_discovery_response(can_node_id_t to)
{
	uint8_t data[] = {
		(1 << 1) | (1 << 0), // Sensor capability & response
		(config.sn >> 24),
		(config.sn >> 16),
		(config.sn >> 8),
		(config.sn >> 0),
	};
	
	can_node_send(511, to, sizeof(data), data);
}

static void can_node_discovery_callback(const can_frame_t frame, void* context)
{
	// TODO: better don't respond in the isr

	// Request
	if ((frame.data[0] & (1 << 0)) == 0 && (can_id_extract_to(frame.id) == CAN_NODE_BROADCAST_ID || can_id_extract_to(frame.id) == can_node.node_id)) {
		can_node_send_discovery_response(can_id_extract_from(frame.id));
	}
}

#if CAN_NODE_NUM_OUTPUT > 0
static void can_node_sensor_callback(const can_frame_t frame, void* context)
{
	can_node_id_t from = can_id_extract_from(frame.id);

	for (uint8_t i = 0; i < CAN_NODE_NUM_OUTPUT; i++) {
		for (uint8_t j = 0; j < CAN_NODE_NUM_OUTPUT_SOURCE; j++) {
			struct _output_src* src = &can_node.outputs[i].srcs[j];
			struct can_node_output_src_config* conf = &config.can_node.output_src_configs[i * CAN_NODE_NUM_OUTPUT + j];

			if (conf->node_id == from) {
				if (frame.data_length < 6)
					return;

				if (frame.data[0] == conf->sensor_type && frame.data[1] == conf->sensor_idx) {
					int32_t value = (frame.data[2] << 24) | (frame.data[3] << 16) | (frame.data[4] << 8) | (frame.data[5] << 0);

					if (src->state && value < conf->off_value) {
						src->state = false;
						can_node.reqs |= kCanNodeWorkReqOutput;
						semaphore_signal(&can_node.sem);
					}
					else if (!src->state && value > conf->on_value) {
						src->state = true;
						can_node.reqs |= kCanNodeWorkReqOutput;
						semaphore_signal(&can_node.sem);
					}
				}

				return;
			}
		}
	}
}
#endif

status_t can_node_init(can_node_id_t node_id, can_speed_t speed)
{
	assert_can_node_id(node_id);

	status_t err = can_init(speed);

	if (err != STATUS_OK)
		return err;

	can_node.node_id = node_id;

	semaphore_init(&can_node.sem, 0);

	can_node_send_discovery_response(CAN_NODE_BROADCAST_ID);
	can_set_receive_callback(can_id_build(CAN_NODE_BROADCAST_ID, CAN_NODE_BROADCAST_ID, 511),
							 can_id_build(CAN_NODE_BROADCAST_ID, CAN_NODE_BROADCAST_ID, 0x1FF),
							 CAN_FRAME_FLAG_EXT, can_node_discovery_callback, NULL);

#if CAN_NODE_NUM_OUTPUT > 0
	can_set_receive_callback(can_id_build(CAN_NODE_BROADCAST_ID, CAN_NODE_BROADCAST_ID, 500),
							 can_id_build(CAN_NODE_BROADCAST_ID, CAN_NODE_BROADCAST_ID, 0x1FF),
							 CAN_FRAME_FLAG_EXT, can_node_sensor_callback, NULL);
#endif

	return STATUS_OK;
}

status_t can_node_set_output_pin(uint8_t n, pin_t pin)
{
	if (n >= CAN_NODE_NUM_OUTPUT) {
		return STATUS_ERR(0);
	}

	can_node.outputs[n].pin = pin;

	return STATUS_OK;
}

status_t can_node_send(can_node_topic_t topic, can_node_id_t to, uint8_t len, const uint8_t* data)
{
	assert_can_node_id(to);
	assert(len <= 8, "Can node send length must be less than 9");

	can_frame_t frame;
	frame.id = can_id_build(can_node.node_id, to, topic);
	frame.flags = CAN_FRAME_FLAG_EXT;

	frame.data_length = len;
	memcpy(frame.data, data, len);

	return can_send(frame, 0);
}

static bool can_node_loop_publish_sensor(sensor_t sensor, void* context)
{
	uint8_t* idx = context;

	sensor_capabilities_t capabilities = sensor_get_capabilities(sensor);

	if (capabilities & SENSOR_CAPABILITY_TEMP) {
		int32_t t;

		if (sensor_get_temp(sensor, &t) == STATUS_OK) {
			uint8_t data[] = {
				0x0, // Temperature
				*idx, // Index
				(t >> 24),
				(t >> 16),
				(t >> 8),
				(t >> 0),
			};

			log(LOG_LEVEL_DEBUG, "can_node: temp at %d is %d", *idx, t);
			can_node_send(500, CAN_NODE_BROADCAST_ID, sizeof(data), data);
		}
		else {
			log(LOG_LEVEL_WARN, "can_node: can not read temp at %d", *idx);
		}
	}

	if (capabilities & SENSOR_CAPABILITY_HUMIDITY) {
		int32_t rh;

		if (sensor_get_humidity(sensor, &rh) == STATUS_OK) {
			uint8_t data[] = {
				0x1, // RH
				*idx, // Index
				(rh >> 24),
				(rh >> 16),
				(rh >> 8),
				(rh >> 0),
			};
			
			log(LOG_LEVEL_DEBUG, "can_node: hum at %d is %d", *idx, rh);
			can_node_send(500, CAN_NODE_BROADCAST_ID, sizeof(data), data);
		}
		else {
			log(LOG_LEVEL_WARN, "can_node: can not read hum at %d", *idx);
		}
	}

	if (capabilities & SENSOR_CAPABILITY_VOLTAGE) {
		int32_t volt;

		if (sensor_get_voltage(sensor, &volt) == STATUS_OK) {
			uint8_t data[] = {
				0x2, // Voltage
				*idx, // Index
				(volt >> 24),
				(volt >> 16),
				(volt >> 8),
				(volt >> 0),
			};
			
			log(LOG_LEVEL_DEBUG, "can_node: volt at %d is %d", *idx, volt);
			can_node_send(500, CAN_NODE_BROADCAST_ID, sizeof(data), data);
		}
		else {
			log(LOG_LEVEL_WARN, "can_node: can not read volt at %d", *idx);
		}
	}

	(*idx)++;

	return true;
}

static void can_node_post_sensor_req()
{
	can_node.reqs |= kCanNodeWorkReqSensors;
	semaphore_signal(&can_node.sem);
}

void can_node_loop()
{
	timer_managed_schedule(default_timer, config.can_node.sensor_interval, true, can_node_post_sensor_req, NULL);

	for (;;) {
		semaphore_wait(&can_node.sem);

		can_node_work_req_t reqs = can_node_fetch_clear_reqs();

		if (reqs & kCanNodeWorkReqSensors) {
			uint8_t idx = 0;
			sensors_for_each(can_node_loop_publish_sensor, &idx);
		}

		if (reqs & kCanNodeWorkReqOutput) {
			bool send_output_status = false;
			uint8_t status[CAN_NODE_NUM_OUTPUT / 8 + 1] = {0};

			for (uint8_t i = 0; i < CAN_NODE_NUM_OUTPUT; i++) {
				bool state = false;
				for (uint8_t j = 0; j < CAN_NODE_NUM_OUTPUT_SOURCE; j++) {
					if (can_node.outputs[i].srcs[j].state) {
						state = true;
						break;
					}
				}

				if (can_node.outputs[i].state != state) {
					can_node.outputs[i].state = state;
					gpio_set(can_node.outputs[i].pin, state);
					send_output_status = true;
				}

				status[i / 8] |= (state << (7 - (i % 8)));
			}

			can_node_send(499, CAN_NODE_BROADCAST_ID, sizeof(status), status);
		}
	}
}

// struct config_val_desc {
// 	const char* name;
// 	uint8_t idx;

// 	off_t offset;
// 	uint8_t type; // Array
// 	uint8_t element_size;

// 	struct config_val_desc[] subdescs; 
// };

// can_node_id_t node_id;
// 	uint8_t sensor_type;
// 	uint8_t sensor_idx;
// 	int32_t off_value;
// 	int32_t on_value;

static const struct config_val_desc can_config_desc_2[] = {
	{
		.name = "node_id",
		.idx = 1,

		.offset = offsetof(struct can_node_output_src_config, node_id),
		.type = kConfigValTypeUInt16, // uint16
		.flags = kConfigValFlagConsoleHex,

		.subdescs = NULL
	},
	{
		.name = "sensor_type",
		.idx = 2,

		.offset = offsetof(struct can_node_output_src_config, sensor_type),
		.type = kConfigValTypeUInt8, // uint8

		.subdescs = NULL
	},
	{
		.name = "sensor_idx",
		.idx = 2,

		.offset = offsetof(struct can_node_output_src_config, sensor_idx),
		.type = kConfigValTypeUInt8, // uint8

		.subdescs = NULL
	},
	{
		.name = "off_value",
		.idx = 3,

		.offset = offsetof(struct can_node_output_src_config, off_value),
		.type = kConfigValTypeUInt32, // uint8

		.subdescs = NULL
	},
	{
		.name = "on_value",
		.idx = 4,

		.offset = offsetof(struct can_node_output_src_config, on_value),
		.type = kConfigValTypeUInt32, // uint8

		.subdescs = NULL
	},
	{ .name = NULL }
};

static const struct config_val_desc can_config_desc_1[] = {
	{
		.name = "node_id",
		.idx = 1,

		.offset = offsetof(struct can_node_config, node_id),
		.type = kConfigValTypeUInt16, // uint16
		.flags = kConfigValFlagConsoleHex,

		.subdescs = NULL
	},
	{
		.name = "speed",
		.idx = 2,

		.offset = offsetof(struct can_node_config, speed),
		.type = kConfigValTypeUInt32, // uint32

		.subdescs = NULL
	},
	{
		.name = "sensor_interval",
		.idx = 3,

		.offset = offsetof(struct can_node_config, sensor_interval),
		.type = kConfigValTypeUInt32, // uint32

		.subdescs = NULL
	},
#if CAN_NODE_NUM_OUTPUT > 0
	{
		.name = "output_src",
		.idx = 4,

		.offset = offsetof(struct can_node_config, output_src_configs),
		.type = kConfigValTypeArray, // array
		.element_size = sizeof(struct can_node_output_src_config),
		.element_count = CAN_NODE_NUM_OUTPUT_SOURCE * CAN_NODE_NUM_OUTPUT,

		.subdescs = can_config_desc_2,
	},
#endif
	{ .name = NULL },
};

CONFIG_VAL_ROOT_DESC(can) = {
	.name = "can", 
	.idx = 10,

	.offset = offsetof(struct config_t, can_node),
	.type = kConfigValTypeStruct, // Struct

	.subdescs = can_config_desc_1,
};

// CONFIG_VAL_DESC(can.node_id, can_node.node_id, kConfigValTypeUInt16, kConfigValFlagConsoleHex, NULL, NULL);
// CONFIG_VAL_DESC(can.speed, can_node.speed, kConfigValTypeUInt32, 0, NULL, NULL);

// static status_t can_sensor_intervall_set_cb(uint32_t* dest, uint32_t val)
// {
// 	*dest = val * 1000;
// 	return STATUS_OK;
// }

// static uint32_t can_sensor_intervall_get_cb(uint32_t val)
// {
// 	return val/1000;
// }

// CONFIG_VAL_DESC(can.sensor_interval, can_node.sensor_interval, kConfigValTypeUInt32, 0, can_sensor_intervall_set_cb, can_sensor_intervall_get_cb);

