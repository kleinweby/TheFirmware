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

#include <sht2x.h>
#include <string.h>
#include <console.h>

static void sht2x_cmd_help()
{
	printf("sht2x\n");
	printf("  read - read temperature and humidity\n");
	printf("  id   - read serial number\n");
}

static sht2x_t sht21 = NULL;

int sht2x_cmd(int argc, const char** argv)
{
	if (argc != 1) {
		sht2x_cmd_help();
		return -1;
	}

	if (!sht21) {
		i2c_dev_t i2c = i2c_dev_create();
		sht21 = sht2x_create(i2c);
	}

	if (strcmp(argv[0], "read") == 0) {
		printf("Temp: ");
		int16_t t = sht2x_measure_temperature(sht21);
		printf("%d.%03d ºC\nRH: ", t/1000, t % 1000);
		int16_t rh = sht2x_measure_humidity(sht21);
		printf("%d.%03d %%RH\n", rh, rh/1000, rh % 1000);
	}
	else if (strcmp(argv[0], "id") == 0) {
		uint64_t id = sht2x_read_serial_number(sht21);
		printf("Serial number: %0x%08x", (uint32_t)((id >> 32) & 0xFFFFFFFF), (uint32_t)(id & 0xFFFFFFFF));
	}
	else {
		sht2x_cmd_help();
		return -1;
	}
	
	return 0;
}
