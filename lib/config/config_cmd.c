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

#include <config/config.h>
#include <console.h>
#include <string.h>

CONFIG_VAL_DESC(sn, sn, kConfigValTypeUInt32, 0, NULL, NULL);

LINKER_SYMBOL(config_val_decls_begin, struct config_val_desc*);
LINKER_SYMBOL(config_val_decls_end, struct config_val_desc*);

static status_t config_cmd_set_from_str(const struct config_val_desc* d, const char* str)
{
	switch(d->type) {
		case kConfigValTypeUInt8:
		{
			uint8_t* dest = OFFSET_PTR(&config, d->offset);

			uint8_t val = strtol(str, NULL, 0);

			if (d->set_cb) {
				status_t (*set_cb)(uint8_t*, uint8_t) = d->set_cb;
				return set_cb(dest, val);
			}
			else {
				*dest = val;

				return STATUS_OK;
			}
		}
		case kConfigValTypeUInt16:
		{
			uint16_t* dest = OFFSET_PTR(&config, d->offset);

			uint16_t val = strtol(str, NULL, 0);

			if (d->set_cb) {
				status_t (*set_cb)(uint16_t*, uint16_t) = d->set_cb;
				return set_cb(dest, val);
			}
			else {
				*dest = val;

				return STATUS_OK;
			}
		}
		case kConfigValTypeUInt32:
		{
			uint32_t* dest = OFFSET_PTR(&config, d->offset);

			uint32_t val = strtol(str, NULL, 0);

			if (d->set_cb) {
				status_t (*set_cb)(uint32_t*, uint32_t) = d->set_cb;
				return set_cb(dest, val);
			}
			else {
				*dest = val;

				return STATUS_OK;
			}
		}
	}

	return STATUS_ERR(0);
}

static void config_cmd_val_print(const struct config_val_desc* d)
{
	printf("%s = ", d->name);

	const char* fmt = "%d";

	if (d->flags & kConfigValFlagConsoleHex) {
		fmt = "%x";
	}

	switch(d->type) {
		case kConfigValTypeUInt8:
		{
			uint8_t* val = OFFSET_PTR(&config, d->offset);

			if (d->get_cb) {
				uint8_t (*get_cb)(uint8_t) = d->get_cb;
				printf(fmt, get_cb(*val));
			}
			else {
				printf(fmt, *val);
			}
			break;
		}
		case kConfigValTypeUInt16:
		{
			uint16_t* val = OFFSET_PTR(&config, d->offset);

			if (d->get_cb) {
				uint16_t (*get_cb)(uint16_t) = d->get_cb;
				printf(fmt, get_cb(*val));
			}
			else {
				printf(fmt, *val);
			}
			break;
		}
		case kConfigValTypeUInt32:
		{
			uint32_t* val = OFFSET_PTR(&config, d->offset);

			if (d->get_cb) {
				uint32_t (*get_cb)(uint32_t) = d->get_cb;
				printf(fmt, get_cb(*val));
			}
			else {
				printf(fmt, *val);
			}
			break;
		}
	}

	printf("\r\n");
}

int config_cmd(int argc, const char** argv)
{
	if (argc < 2) {
		for (const struct config_val_desc* d = config_val_decls_begin; d < config_val_decls_end; d++) {
			config_cmd_val_print(d);
		}

		return 0;
	}

	if (strcmp("save", argv[1]) == 0) {
		config_save();
		return 0;
	}
	else if (strcmp("load", argv[1]) == 0) {
		config_load();
		return 0;
	}

	const struct config_val_desc* desc = NULL;

	for (const struct config_val_desc* d = config_val_decls_begin; d < config_val_decls_end; d++) {
		if (strcmp(d->name, argv[1]) == 0) {
			desc = d;
			break;
		}
	}

	if (desc == NULL) {
		printf("unkown config var \r\n");
		return -1;
	}

	if (argc == 3) {
		if (config_cmd_set_from_str(desc, argv[2]) != STATUS_OK) {
			return -1;
		}
	}

	config_cmd_val_print(desc);

	return 0;
}

CONSOLE_CMD(config, config_cmd);

