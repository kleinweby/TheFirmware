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

//
// Much inspiration from github.com/dwelch67/thumbulator
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <ev.h>

#include <time.h>

#include <mcu.h>
#include <gdb.h>
#include <elf.h>
#include <mcu_kernel.h>

bool mcu_flash_file(mcu_t mcu, const char* filename)
{
	return elf_load(mcu, filename);
}

int main(int argc, char** argv) {
	mcu_kernel_init();

	struct ev_loop *loop = EV_DEFAULT;

	bool wait_for_gdb = false;
	int gdb_port = 1234;
	const char* firmware_file = NULL;
	char ch;

	mcu_t mcu;
	gdb_t gdb;

	while ((ch = getopt(argc, argv, "gp:f:")) != -1) {
		switch (ch) {
			case 'g':
				wait_for_gdb = true;
				break;
			case 'p':
				gdb_port = atol(optarg);
				break;
			case 'f':
				firmware_file = optarg;
				break;
			case '?':
				printf("%s - MCU Simulator\n", argv[0]);
				printf("  -g wait for debugger when mcu halts\n");
				printf("  -G wait for debugger to attach\n");
				break;
		}
	}

	mcu = mcu_cortex_m0p_create(loop, 8 * 1024);

	if (!mcu) {
		printf("Could not create mcu\n");
		return -1;
	}

	gdb = gdb_create(loop, gdb_port, mcu);

	if (!gdb) {
		printf("Could not create gdb\n");
		return -1;
	}

	if (firmware_file) {
		if (!mcu_flash_file(mcu, firmware_file)) {
			printf("Flash faild");
			return -1;
		}
	}

	if (!mcu_reset(mcu)) {
		printf("MCU reset failed");
		return -1;
	}

	if (!wait_for_gdb)
		mcu_resume(mcu);

	ev_run(loop, 0);

	return 0;
}
