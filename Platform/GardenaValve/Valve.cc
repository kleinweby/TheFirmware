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

#include "Valve.h"

#include "Firmware/Console/Console.h"
#include "Firmware/Console/CLI.h"
#include "Firmware/Console/string.h"
#include "Firmware/GPIO.h"
#include "Firmware/Time/Delay.h"

using namespace TheFirmware::Console;
using namespace TheFirmware;
using TheFirmware::GPIO;
using namespace TheFirmware::Time;

namespace GardenaValve {

GPIO onPin(2, 7);
GPIO offPin(2, 6);

millitime_t onTime = 50;
millitime_t offTime = 5;

/// Checks if a given char is a digit
bool isDigit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	
	return false;
}

uint32_t atoi(char* c)
{
	uint32_t i = 0;

	for (; *c != '\0' && isDigit(*c); c++) {
		i = 10 * i + ((*c)-'0');
	}

	return i;
}

void Init()
{
	onPin.setDirection(GPIODirectionOutput);
	offPin.setDirection(GPIODirectionOutput);

	onPin.set(false);
	offPin.set(false);
}

void valve_open()
{
	offPin.set(false);
	onPin.set(true);
	delay(onTime);
	onPin.set(false);
}

void valve_close()
{
	onPin.set(false);

	// Polarity 1
	for (int i = 0; i < 5; i++) {
	offPin.set(true);
	delay(offTime);
	offPin.set(false);

	// When demagnetizing in booth polarity take a reasonable long
	// delay between those pulses
	delay(offTime);
	// Polarity 2
	onPin.set(true);
	delay(offTime);
	onPin.set(false);
	}

}

void valve_usage()
{
	printf("valve [on|off]\r\n");
}

void valve_cmd(int argc, char** argv) 
{
	if (argc < 1) {
		valve_usage();
		return;
	}

	if (strcmp(argv[0], "on") == 0) {
		printf("Opening valve...\r\n");
		valve_open();
	}
	else if (strcmp(argv[0], "off") == 0) {
		printf("Closing valve...\r\n");
		valve_close();
	}
	else if (strcmp(argv[0], "time") == 0) {
		if (argc == 1) {
			printf("On time %dms\r\n", onTime);
			printf("Off time %dms\r\n", offTime);
		}
		else if (argc == 3) {
			onTime = atoi(argv[1]);
			offTime = atoi(argv[2]);
		}
		else 
			valve_usage();
	}
	else
		valve_usage();
}

REGISTER_COMMAND(valve, {
	.func = valve_cmd,
	.help = "Control the valve"
});

} // namespace GardenaValve
