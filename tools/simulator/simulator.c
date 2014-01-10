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

#include <time.h>

#include <mcu.h>
#include <gdb.h>
#include <flash.h>

bool mcu_flash_file(mcu_t mcu, const char* filename)
{
	for (mem_dev_t dev = mcu->mem_devs; dev != NULL; dev = dev->next) {
		if (dev->type == flash_mem_type) {
			return flash_dev_load_bin((flash_dev_t)dev, filename);
		}
	}

	return false;
}

int main(int argc, char** argv) {
	mcu_t mcu = mcu_cortex_m0p_create(8 * 1024);
	gdb_t gdb = gdb_create(1234, mcu);
	
	if (!mcu_flash_file(mcu, "./firmware.bin")) {
		printf("Flash faild");
		return -1;
	}

	if (!mcu_reset(mcu)) {
		printf("MCU reset failed");
		return -1;
	}

	// mcu_resume(mcu);

	int i = 0;
	clock_t start = clock();

	while (true) {
		gdb_runloop(gdb);
		mcu_runloop(mcu);
		i++;
		// printf("step\n");
	}

	clock_t end = clock();
	uint32_t elapsed = (end - start)/ (CLOCKS_PER_SEC / 1000000);
	uint64_t instructions_per_second = 1000000 * i / elapsed;

	printf("Exit [pc=0x%04x]: %u instructions, %fms elapsed, %llu instr/s\n", mcu_read_reg(mcu, REG_PC), i, elapsed/1000.f, instructions_per_second);

	printf("Bla");

	return 0;
}

