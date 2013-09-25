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

using namespace TheFirmware::Console;
using TheFirmware::GPIO;

namespace GardenaValve {

GPIO onPin(2, 7);
GPIO offPin(2, 6);

void Init()
{

}

void valve_usage()
{
	printf("valve [on|off]\r\n");
}

void valve_cmd(int argc, char** argv) 
{
	if (argc != 1) {
		valve_usage();
		return;
	}

	if (strcmp(argv[0], "on") == 0) {
		printf("Opening valve...\r\n");
	}
	else if (strcmp(argv[0], "off") == 0) {
		printf("Closing valve...\r\n");
	}
	else
		valve_usage();
}

REGISTER_COMMAND(valve, {
	.func = valve_cmd,
	.help = "Control the valve"
});

} // namespace GardenaValve
