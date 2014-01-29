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

#include "unittest.h"

#include <mcu.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define COLOR_RESET "\x1B[0m"
#define COLOR_RED  "\x1B[31m"
#define COLOR_GREEN  "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"

enum {
	CURRENT_TEST_OFFSET =  0x0,
	TOTAL_TESTS_OFFSET  =  0x4,
	DESC                =  0x8,
	STATUS_OFFSET       = 0x12,
	STATUS_DESC_OFFSET  = 0x16,

	SIZE                = 0x20,
};

enum {
	STATUS_PREPARING    = 0x55555555,
	STATUS_RUNNING      = 0xDB6DB6DB,

	STATUS_SKIPPED      = 0x24924924,
	STATUS_PASSED       = 0xBEEFBEEF,
	STATUS_FAILED       = 0xDEADBEEF,
};

struct unittest_dev {
	struct mem_dev mem_dev;

	int32_t current_test;
	int32_t total_tests;

	int32_t tests_skipped;
	int32_t tests_failed;

	char* desc;
	uint32_t status;
	char* status_desc;
};

static void unittest_update_progress(unittest_dev_t unittest_dev)
{
	uint32_t digits = ceil(unittest_dev->total_tests / 10.f);
	bool display_status_desc = false;
	uint32_t width = 80 - 4 - 2*digits;
	uint32_t padd;
	char* status = "____";

	printf("\r[%*d/%d] %.*s%n", digits, unittest_dev->current_test + 1, unittest_dev->total_tests, width, unittest_dev->desc ?: "", &padd);

	padd = 80 - padd;

	if (unittest_dev->status == STATUS_SKIPPED) {
		status = COLOR_YELLOW "SKIP\n" COLOR_RESET;
		display_status_desc = true;
	}
	else if (unittest_dev->status == STATUS_PASSED) {
		status = COLOR_GREEN "PASS\n" COLOR_RESET;
		display_status_desc = true;
	}
	else if (unittest_dev->status == STATUS_FAILED) {
		status = COLOR_RED "FAIL\n" COLOR_RESET;
		display_status_desc = true;
	}

	printf("%*s%s", padd, "", status);

	if (display_status_desc && unittest_dev->status_desc) {
		printf("\t%s\n", unittest_dev->status_desc);
	}
	fflush(stdout);
}

static char* unittest_fetch_str(mcu_t mcu, uint32_t addr)
{
	if (addr == 0x0)
		return NULL;

	uint32_t length = 1024;
	uint32_t used = 0;
	char* str = malloc(length);
	char* ptr = str;

	if (!str) {
		perror("malloc");
		return NULL;
	}

	do {
		if (!mcu_util_fetch8(mcu, addr, (uint8_t*)ptr)) {
			free(str);
			return NULL;
		}

		if (*ptr == '\0')
			break;

		used++;
		addr++;
		ptr++;
	} while (used < length);

	return str;
}

static bool unittest_dev_read32(mcu_t mcu, mem_dev_t _mem_dev, uint32_t addr, uint32_t* temp) {
	unittest_dev_t mem_dev = (unittest_dev_t)_mem_dev;

	switch(addr) {
		case CURRENT_TEST_OFFSET:
			*temp = mem_dev->current_test;
			break;
		case TOTAL_TESTS_OFFSET:
			*temp = mem_dev->total_tests;
			break;
		default:
			return false;
	}

	return true;
}

static bool unittest_done(unittest_dev_t mem_dev)
{
	printf("%d run, %d skipped, "COLOR_RED"%d failed\n"COLOR_RESET, mem_dev->total_tests, mem_dev->tests_skipped, mem_dev->tests_failed);
	exit(mem_dev->tests_failed > 0 ? -1 : 0);
}

static bool unittest_dev_write32(mcu_t mcu, mem_dev_t _mem_dev, uint32_t addr, uint32_t temp)
{
	unittest_dev_t mem_dev = (unittest_dev_t)_mem_dev;

	switch(addr) {
		case CURRENT_TEST_OFFSET:
			return false;
		case TOTAL_TESTS_OFFSET:
			mem_dev->total_tests = temp;
			mem_dev->current_test = 0;
			mem_dev->tests_failed = 0;
			mem_dev->tests_skipped = 0;
			unittest_update_progress(mem_dev);
			mcu_reset(mcu);
			break;
		case DESC:
			if (mem_dev->desc) {
				free(mem_dev->desc);
				mem_dev->desc = NULL;
			}
			mem_dev->desc = unittest_fetch_str(mcu, temp);
			break;
		case STATUS_OFFSET:
			mem_dev->status = temp;
			unittest_update_progress(mem_dev);

			if (temp == STATUS_SKIPPED || temp == STATUS_PASSED || temp == STATUS_FAILED) {
				mem_dev->current_test++;

				if (temp == STATUS_SKIPPED)
					mem_dev->tests_skipped++;
				else if (temp == STATUS_FAILED)
					mem_dev->tests_failed++;

				if (mem_dev->current_test < mem_dev->total_tests) {
					mem_dev->desc = NULL;
					mcu_reset(mcu);
				}
				else {
					unittest_done(mem_dev);
				}
			}
			break;
		case STATUS_DESC_OFFSET:
			if (mem_dev->status_desc) {
				free(mem_dev->status_desc);
				mem_dev->status_desc = NULL;
			}
			mem_dev->status_desc = unittest_fetch_str(mcu, temp);
			break;
		default:
			return false;
	}

	return true;
}

unittest_dev_t unittest_dev_create()
{
	unittest_dev_t dev = calloc(1, sizeof(struct unittest_dev));

	if (!dev) {
		perror("Could not allocate unittest_dev structure");
		return NULL;
	}

	dev->mem_dev.class = mem_class_io;
	dev->mem_dev.type = unittest_mem_type;
	dev->mem_dev.fetch16 = mcu_emu_fetch16;
	dev->mem_dev.fetch32 = unittest_dev_read32;
	dev->mem_dev.write16 = mcu_emu_write16;
	dev->mem_dev.write32 = unittest_dev_write32;
	dev->mem_dev.length = SIZE;

	dev->current_test = -1;
	dev->total_tests = 0;

	return dev;
}
