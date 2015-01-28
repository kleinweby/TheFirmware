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
#include <scheduler.h>
#include "LPC11xx.h"

static void sht2x_cmd_help()
{
	printf("sht2x:\r\n");
	printf("  read       - read temperature and humidity\r\n");
	printf("  id         - read serial number\r\n");
	printf("  config get -\r\n");
	printf("  reset      -\r\n");
}

static sht2x_t sht21 = NULL;

int sht2x_cmd(int argc, const char** argv)
{
	if (argc < 2) {
		sht2x_cmd_help();
		return -1;
	}

	if (!sht21) {
		i2c_dev_t i2c = i2c_dev_create();
		sht21 = sht2x_create(i2c);
	}

	if (strcmp(argv[1], "read") == 0) {
		uint32_t repeatCount = 1;
		millitime_t waitDuration = 0;

		if (argc > 2) {
			waitDuration = atoi(argv[2]) * 1000;

			if (argc > 3) {
				repeatCount = atoi(argv[3]);
			}
			else {
				repeatCount = UINT32_MAX;
			}
		}

		while(repeatCount > 0) {
			int32_t t;

			if (sensor_get_temp((sensor_t)sht21, &t) != STATUS_OK) {
				printf("Reading t faild\r\n");
				continue;
			}

			int32_t rh;

			if (sensor_get_humidity((sensor_t)sht21, &rh) != STATUS_OK) {
				printf("Reading rh faild\r\n");
				continue;
			}
			printf("Temp: %d.%03d ºC RH: %d.%03d %%RH\r\n", t/1000, t % 1000, rh/1000, rh % 1000);

			if (waitDuration) {
				delay(waitDuration);
			}
			repeatCount--;
		}
	}
	else if (strcmp(argv[1], "id") == 0) {
		uint64_t id = sht2x_read_serial_number(sht21);
		printf("Serial number: %0x%08X\r\n", (uint32_t)((id >> 32) & 0xFFFFFFFF), (uint32_t)(id & 0xFFFFFFFF));
	}
	else if (strcmp(argv[1], "config") == 0) {
		if (argc < 3) {
			sht2x_cmd_help();
			return -1;
		}

		if (strcmp(argv[2], "get") == 0) {
			uint8_t conf = sht2x_read_config(sht21);

			printf("Conf: %x\r\n", conf);
			uint8_t resolution = ((conf >> 7) & 0x1) | (conf & 0x1);

			switch (resolution) {
				case 0x0:
					printf("  Resolution: 12bit RH, 14bit T\r\n");
					break;
				case 0x1:
					printf("  Resolution: 8bit RH, 12bit T\r\n");
					break;
				case 0x2:
					printf("  Resolution: 10bit RH, 13bit T\r\n");
					break;
				case 0x3:
					printf("  Resolution: 11bit RH, 11bit T\r\n");
					break;
			}

			if ((conf >> 6) & 0x1) {
				printf("  End of battery: < 2.25V\r\n");
			}
			else {
				printf("  End of battery: > 2.25V\r\n");
			}

			if ((conf >> 2) & 0x1) {
				printf("  Heater: ON\r\n");
			}
			else {
				printf("  Heater: OFF\r\n");
			}

			if ((conf >> 1) & 0x1) {
				printf("  OTP: OFF\r\n");
			}
			else {
				printf("  OTP: ON\r\n");
			}
		}
	}
	else if (strcmp(argv[1], "reset") == 0) {
		sht2x_soft_reset(sht21);
		printf("Sensor has been reset.\r\n");
		return 0;
	}
	else {
		sht2x_cmd_help();
		return -1;
	}
	
	return 0;
}

int reset_cmd(int argc, const char** argv)
{
	if (argc != 2) {
		return -1;
	}

	if (strcmp(argv[1], "reason") == 0) {
		uint32_t val = LPC_SYSCON->SYSRESSTAT;

		printf("Reset reason: ");
		if (val & (1 << 0)) {
			printf("POR ");
		}
		if (val & (1 << 1)) {
			printf("EXT ");
		}
		if (val & (1 << 2)) {
			printf("WDT ");
		}
		if (val & (1 << 3)) {
			printf("BOD ");
		}
		if (val & (1 << 4)) {
			printf("SYS ");
		}
		printf("\r\n");
		return 0;
	}
	else if (strcmp(argv[1], "clear") == 0) {
		LPC_SYSCON->SYSRESSTAT = LPC_SYSCON->SYSRESSTAT;
		return 0;
	}

	return -1;
}
