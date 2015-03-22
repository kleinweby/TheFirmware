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

#include <can_node.h>
#include <runtime.h>
#include <string.h>

struct can_node_output_src_config {
	can_node_id_t node_id;
	uint8_t sensor_type;
	uint8_t sensor_idx;
	int32_t off_value;
	int32_t on_value;
};

typedef ENUM(uint8_t, can_node_output_mode_t) {
	kCanNodeOutputModeAuto = 0,
	kCanNodeOutputModeManual = 1,
};

struct can_node_output_config {
	can_node_output_mode_t mode;

#if CAN_NODE_NUM_OUTPUT_SOURCE
	struct can_node_output_src_config sources[CAN_NODE_NUM_OUTPUT_SOURCE];
#endif
};

struct can_node_config {
	can_node_id_t node_id;
	can_speed_t speed;
	millitime_t sensor_interval;

#if CAN_NODE_NUM_OUTPUT > 0
	struct can_node_output_config outputs[CAN_NODE_NUM_OUTPUT];
#endif
};

static ALWAYS_INLINE void can_node_config_defaults(struct can_node_config* conf)
{
	conf->node_id = 0;
	conf->speed = 125000;
	conf->sensor_interval = 30;

	memset(&conf->outputs, 0, sizeof(struct can_node_output_config));
}
