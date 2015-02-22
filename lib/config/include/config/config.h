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

#if HAVE_CAN_NODE
#include <config/can_node.h>
#endif

#include <dev/eeprom.h>
#include <timer.h>

struct config_t {
	uint32_t sn;
	
#if HAVE_CAN_NODE
	struct can_node_config can_node;
#endif
};

extern eeprom_t config_eeprom;
void config_init(eeprom_t eeprom);

void config_load();
void config_load_defaults();
void config_save();

extern struct config_t config;

// CONFIG_VAL_DESC(can.node_id, can_node.node_id, kConfigTypeUInt8, READONLY | LOCKED)

typedef ENUM(uint8_t, config_val_type_t) {
	kConfigValTypeUInt8,
	kConfigValTypeUInt16,
	kConfigValTypeUInt32,
};

typedef ENUM(uint32_t, config_val_flags_t) {
	// Display value as hex
	kConfigValFlagConsoleHex = (1 << 0),
};

struct config_val_desc {
	const char* name;
	off_t offset;
	config_val_type_t type;
	config_val_flags_t flags;

	// Those callbacks are used when interfacing with config commands (via uart, can etc.)
	// NOTICE: Especially the get_cb is not used when loading the config from eeprom
	// Real signature is status_t (*cb)(TYPE* dest, TYPE new_val)
	void* set_cb;
	// Real signature is TYPE (*cb)(TYPE val)
	void* get_cb;
};

// TODO: the var name may not be unique :/ better idea?
#define CONFIG_VAL_DESC(_name, _member, _type, _flags, _set_cb, _get_cb) \
	const struct config_val_desc CONCAT(CONCAT(__config_val_desc, __LINE__), __COUNTER__) __attribute__ ((section (".config_val_desc." #_name))) = { \
		.name = #_name, \
		.offset = offsetof(struct config_t, _member), \
		.type = _type, \
		.flags = _flags, \
		.set_cb = _set_cb, \
		.get_cb = _get_cb, \
	}
