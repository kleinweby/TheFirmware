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

static void config_cmd_help()
{
	printf("config:\r\n");
	printf("  sn   - Serial Number\r\n");
	printf("  can  - CAN related settings\r\n");
	printf("\r\n");
	printf("  load - Load settings from eeprom\r\n");
	printf("  save - Write settings to eeprom\r\n");
}

static void config_cmd_can_help()
{
	printf("config can:\r\n");
	printf("  node_id         - CAN node id\r\n");
	printf("  speed           - interface speed\r\n");
	printf("  sensor_interval - Interval for sensor reports (seconds)\r\n");
}

int config_cmd(int argc, const char** argv)
{
	if (argc < 2) {
		config_cmd_help();
		return -1;
	}

	if (strcmp(argv[1], "sn") == 0) {
		if (argc == 3) {
			config.sn = strtol(argv[2], NULL, 0);
		}
		else if (argc != 2) {
			config_cmd_can_help();
			return -1;
		}

		printf("sn = %d\r\n", config.sn);
	}
	else if (strcmp(argv[1], "can") == 0) {
		if (argc < 3) {
			printf("can node_id = %x\r\n", config.can_node.node_id);
			printf("can speed = %d\r\n", config.can_node.speed);
			printf("can sensor_interval = %ds\r\n", config.can_node.sensor_interval/1000);
			return 0;
		}

		if (strcmp(argv[2], "node_id") == 0) {
			if (argc == 4) {
				config.can_node.node_id = strtol(argv[3], NULL, 0);
			}
			else if (argc != 3) {
				config_cmd_can_help();
				return -1;
			}

			printf("can node_id = %x\r\n", config.can_node.node_id);
		}
		else if (strcmp(argv[2], "speed") == 0) {
			if (argc == 4) {
				config.can_node.speed = strtol(argv[3], NULL, 0);
			}
			else if (argc != 3) {
				config_cmd_can_help();
				return -1;
			}

			printf("can speed = %d\r\n", config.can_node.speed);
		}
		else if (strcmp(argv[2], "sensor_interval") == 0) {
			if (argc == 4) {
				config.can_node.sensor_interval = strtol(argv[3], NULL, 0) * 1000;
			}
			else if (argc != 3) {
				config_cmd_can_help();
				return -1;
			}

			printf("can sensor_interval = %ds\r\n", config.can_node.sensor_interval/1000);
		}
		else {
			config_cmd_can_help();
			return -1;
		}
	}
	else if (strcmp(argv[1], "load") == 0) {
		if (argc == 3 && strcmp(argv[2], "defaults") == 0) {
			config_load_defaults();
			printf("Loaded defaults\r\n");
		}
		else {
			config_load();
			printf("Config loaded\r\n");
		}
	}
	else if (strcmp(argv[1], "save") == 0) {
		config_save();
		printf("Config saved\r\n");
	}
	else {
		config_cmd_help();
		return -1;
	}

	return 0;
}

CONSOLE_CMD(config, config_cmd);

