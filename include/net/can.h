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

#pragma once

#if !HAVE_CAN
#error CAN support not enabled
#endif

#include <stdint.h>
#include <stdbool.h>
#include <timer.h>
#include <runtime.h>

typedef uint32_t can_speed_t;
typedef uint32_t can_id_t;

typedef ENUM(uint8_t, can_frame_flag_t) {
	// Extended 29bit identifiers
	CAN_FRAME_FLAG_EXT = (1 << 0),
};

// TODO: maybe we should make the acutal structure
// platform dependent to avoid exessive reformatting of the data
// while it traverses the api bounds
typedef struct can_frame {
	can_id_t id;
	can_frame_flag_t flags;
	uint8_t data_length;
	uint8_t data[8];
} can_frame_t;

typedef ENUM(uint8_t, can_flags_t) {
	CAN_FLAG_NOWAIT = (1 << 0), // Only valid for can_send
};

status_t can_init(can_speed_t speed);
void can_reset();

void can_set_restart_ms(millitime_t time);

status_t can_send(const can_frame_t frame, can_flags_t flags);
// status_t can_receive(can_id_t id, can_id_t id_mask, can_frame_t* frame, can_flags_t flags);

typedef void (*can_receive_callback_t)(const can_frame_t, void* context);
status_t can_set_receive_callback(can_id_t id, can_id_t id_mask, can_frame_flag_t flags, can_receive_callback_t callback, void* context);
status_t can_unset_receive_callback(can_id_t id, can_id_t id_mask, can_frame_flag_t flags, can_receive_callback_t callback, void* context);
