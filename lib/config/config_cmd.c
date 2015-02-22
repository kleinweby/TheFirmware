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

LINKER_SYMBOL(config_val_decls_begin, struct config_val_desc*);
LINKER_SYMBOL(config_val_decls_end, struct config_val_desc*);

#define PRINT_IDENT() for (uint8_t i = 0; i < ident; i++) { printf("  "); }
static void config_cmd_print_tree(const struct config_val_desc* desc, uint8_t ident, void* conf) {
	PRINT_IDENT();

	if (desc->name) {
		printf("%s = ", desc->name);
	}
	else {
		printf("%i = ", desc->idx);
	}

	const char* fmt = "%d";

	if (desc->flags & kConfigValFlagConsoleHex)
		fmt = "%x";

	if (desc->type == kConfigValTypeStruct) {
		printf("{\r\n");

		for (const struct config_val_desc* d = desc->subdescs; d->name != NULL; d++) {
			config_cmd_print_tree(d, ident + 1, OFFSET_PTR(conf, d->offset));
		}

		PRINT_IDENT();
		printf("}");
	}
	else if (desc->type == kConfigValTypeArray) {
		printf("[\r\n");

		ident++;

		for (uint8_t i = 0; i < desc->element_count; i++) {
			struct config_val_desc d = {
				.name = NULL,
				.idx = i,

				.offset = i * desc->element_size,
				.type = kConfigValTypeStruct,
				.subdescs = desc->subdescs,
			};

			config_cmd_print_tree(&d, ident + 1, OFFSET_PTR(conf, d.offset));
		}

		ident--;
		PRINT_IDENT();
		printf("]");
	}
	else if (desc->type == kConfigValTypeUInt8) {
		printf(fmt, *(uint8_t*)conf);
	}
	else if (desc->type == kConfigValTypeUInt16) {
		printf(fmt, *(uint16_t*)conf);
	}
	else if (desc->type == kConfigValTypeUInt32) {
		printf(fmt, *(uint32_t*)conf);
	}
	else {
		printf("<unkown>");
	}

	printf("\r\n");
}

static void config_cmd_set_get(const struct config_val_desc* desc, const char* remaining, const char* full, const char* arg, void* conf)
{
	// We need to decent further
	if (remaining != NULL) {
		if (desc->type == kConfigValTypeStruct) {
			const char* next = strchr(remaining, '.');
			size_t len = next == NULL ? strlen(remaining) : next - remaining;

			if (next)
				next++;

			for (const struct config_val_desc* d = desc->subdescs; d->name != NULL; d++) {
				if (strncmp(d->name, remaining, len) == 0) {
					config_cmd_set_get(d, next, full, arg, OFFSET_PTR(conf, d->offset));
					return;
				}
			}

			printf("%s is unkown\r\n", remaining);
		}
		else if (desc->type == kConfigValTypeArray) {
			char* next = NULL;
			uint8_t idx = strtol(remaining, &next, 0);

			if (!(*next != '.' || *next != '\0')) {
				printf("Expected an int not %s", remaining);
				return;
			}

			if (*next == '\0')
				next = NULL;
			else
				next++;

			for (uint8_t i = 0; i < desc->element_count; i++) {
				if (i == idx) {
					struct config_val_desc d = {
						.name = NULL,
						.idx = i,

						.offset = i * desc->element_size,
						.type = kConfigValTypeStruct,
						.subdescs = desc->subdescs,
					};

					config_cmd_set_get(&d, next, full, arg, OFFSET_PTR(conf, d.offset));
				}
			}
		}
		else {
			printf("%s is unkown\r\n", remaining);
		}
	}
	else {
		if (desc->type == kConfigValTypeUInt8) {
			if (arg != NULL)
				*(uint8_t*)conf = strtol(arg, NULL, 0);
		}
		else if (desc->type == kConfigValTypeUInt16) {
			if (arg != NULL)
				*(uint16_t*)conf = strtol(arg, NULL, 0);
		}
		else if (desc->type == kConfigValTypeUInt32) {
			if (arg != NULL)
				*(uint32_t*)conf = strtol(arg, NULL, 0);
		}
		else {
			if (arg != NULL)
				printf("%s is not a settable type\r\n", full);
		}

		config_cmd_print_tree(desc, 0, conf);
	}
}

int config_cmd(int argc, const char** argv)
{
	if (argc < 2) {
		for (const struct config_val_desc* d = config_val_decls_begin; d < config_val_decls_end; d++) {
			config_cmd_print_tree(d, 0, OFFSET_PTR(&config, d->offset));
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

	const char* next = strchr(argv[1], '.');
	size_t len = next == NULL ? strlen(argv[1]) : next - argv[1];

	if (next)
		next++;

	for (const struct config_val_desc* d = config_val_decls_begin; d < config_val_decls_end; d++) {
		if (strncmp(d->name, argv[1], len) == 0) {
			desc = d;
			break;
		}
	}

	if (desc == NULL) {
		printf("%s is unkown\r\n", argv[1]);
		return -1;
	}

	config_cmd_set_get(desc, next, argv[1], argc == 3 ? argv[2] : NULL, OFFSET_PTR(&config, desc->offset));

	// config_cmd_val_print(desc);

	return 0;
}

CONSOLE_CMD(config, config_cmd);

