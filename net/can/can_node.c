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

#include <can_node.h>

#include <string.h>

#define assert_can_node_id(id) assert((id & 0xFC00) == 0, "Can node id can only be 10 bit.")

struct _can_node {
	can_node_id_t node_id;

};

static struct _can_node can_node;

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

status_t can_node_init(can_node_id_t node_id, can_speed_t speed)
{
	assert_can_node_id(node_id);

	status_t err = can_init(speed);

	if (err != STATUS_OK)
		return err;

	can_node.node_id = node_id;

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
