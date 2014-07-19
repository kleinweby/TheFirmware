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

#include "bootstrap.h"

#include <arch.h>
#include <irq.h>

#include <stdint.h>

#include <test.h>
#include <log.h>
#include <printk.h>
#include <malloc.h>
#include <thread.h>
#include <scheduler.h>
#include <string.h>
#include <systick.h>
#include <console.h>

#include "LPC11xx.h"
#include "system_LPC11xx.h"

void* test_staticnode = NULL;

int hello(int argc, const char** argv) {
	log(LOG_LEVEL_INFO, "Hello");
	return 0;
}

size_t printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	size_t len = vfprintf(debug_serial, format, args);
	va_end(args);

	return len;
}

void valve_help()
{
	printf("HELLP!!!\r\n");
}

// int valve_cmd(int argc, const char** argv)
// {
// 	if (argc < 2) {
// 		valve_help();
// 		return -1;
// 	}

// 	if (strcmp(argv[1], "help") == 0) {
// 		valve_help();
// 		return 0;
// 	}
// 	else if (strcmp(argv[1], "open") == 0) {
// 	}
// 	else if (strcmp(argv[1], "close") == 0) {
// 	}
// 	else if (strcmp(argv[1], "show") == 0) {
// 		printf("open-power: %d\r\n", valve.open_power);
// 		printf("close-power: %d\r\n", valve.close_power);
// 		printf("polarity: %d\r\n", valve.polarity);
// 	}
// 	else if (strcmp(argv[1], "set") == 0) {
// 		if (argc != 4) {
// 			valve_help();
// 			return -1;
// 		}

// 		if (strcmp(argv[2], "open-power") == 0) {
// 			valve.open_power = atoi(argv[3]);
// 			printf("open-power: %d\r\n", valve.open_power);
// 		}
// 		else if (strcmp(argv[2], "close-power") == 0) {
// 			valve.close_power = atoi(argv[3]);
// 			printf("close-power: %d\r\n", valve.close_power);
// 		}
// 		else if (strcmp(argv[2], "polarity") == 0) {
// 			valve.polarity = atoi(argv[3]);
// 			printf("polarity: %d\r\n", valve.polarity);
// 		}
// 		else
// 			printf("Unkown\r\n");
// 	}
// 	else {
// 		valve_help();
// 	}

// 	return 0;
// }

void error()
{
	assert(false, "some error");
}

void bootstrap()
{
	arch_early_init();
	test_do(TEST_AFTER_ARCH_EARLY_INIT);

	irq_register(IRQ_HARDFAULT, error);

	printk_init(38400);
	irq_init();

	log(LOG_LEVEL_INFO, "Starting up TheFirmware...");

	arch_late_init();
	test_do(TEST_AFTER_ARCH_LATE_INIT);

	thread_init();
	scheduler_init();

	size_t free_mem = get_free_size();

	log(LOG_LEVEL_INFO, "Bootstrap complete. (%u free)", free_mem);

	test_do(TEST_IN_MAIN_TASK);

	staticfs_init();
	vfs_dump(debug_serial);

	console_spawn(debug_serial);

	while(1)
		yield(); //__asm("WFI");
}
