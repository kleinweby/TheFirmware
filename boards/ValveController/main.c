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

#include "valve.h"

#include <string.h>
#include <console.h>

valve_t valve;

int main()
{
	valve = valve_create(PIN(2,7), PIN(2,8), VALVE_GARDENA_9V_TYPE, NULL);
}

void valve_help()
{
	printf("HELLP!!!\r\n");
}

int valve_cmd(int argc, const char** argv)
{
	if (argc < 2) {
 		valve_help();
 		return -1;
	}

	if (strcmp(argv[1], "help") == 0) {
		valve_help();
		return 0;
	}
	else if (strcmp(argv[1], "open") == 0) {
		printf("Open valve...");
		if (valve_open(valve)) {
			printf("ok.\r\n");
		}
		else {
			printf("faild.\r\n");
		}
	}
	else if (strcmp(argv[1], "close") == 0) {
		printf("Close valve...");
		if (valve_close(valve)) {
			printf("ok.\r\n");
		}
		else {
			printf("faild.\r\n");
		}
	}
	// else if (strcmp(argv[1], "show") == 0) {
	// 	printf("open-power: %d\r\n", valve.open_power);
	// 	printf("close-power: %d\r\n", valve.close_power);
	// 	printf("polarity: %d\r\n", valve.polarity);
	// }
	// else if (strcmp(argv[1], "set") == 0) {
	// 	if (argc != 4) {
	// 		valve_help();
	// 		return -1;
	// 	}

	// 	if (strcmp(argv[2], "open-power") == 0) {
	// 		valve.open_power = atoi(argv[3]);
	// 		printf("open-power: %d\r\n", valve.open_power);
	// 	}
	// 	else if (strcmp(argv[2], "close-power") == 0) {
	// 		valve.close_power = atoi(argv[3]);
	// 		printf("close-power: %d\r\n", valve.close_power);
	// 	}
	// 	else if (strcmp(argv[2], "polarity") == 0) {
	// 		valve.polarity = atoi(argv[3]);
	// 		printf("polarity: %d\r\n", valve.polarity);
	// 	}
	// 	else
	// 		printf("Unkown\r\n");
	// }
	else {
		valve_help();
	}

 	return 0;
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
