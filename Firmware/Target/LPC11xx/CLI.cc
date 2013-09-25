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

#include "Firmware/Console/CLI.h"
#include "Firmware/Console/Console.h"

#include "LPC11xx.h"

using namespace TheFirmware::Console;

namespace TheFirmware {
namespace LPC11xx {

#define IAP_LOCATION 0x1fff1ff1
typedef void (*IAP)(unsigned int [],unsigned int[]);
IAP iap_entry = (IAP)IAP_LOCATION;

		unsigned int command[5] = {0, 0, 0, 0, 0}; 
		unsigned int result[5] = {0, 0, 0, 0, 0};

void mcu_info(int argc, char** argv)
{
	{
		command[0] = 54;

		iap_entry(command, result);

		printf("  cpu id = %X\r\n", result[1]);
	}

	{
		command[0] = 55;

		iap_entry(command, result);

		printf("  boot version = %d.%d\r\n", (result[1] >> 0) & 0xFF, (result[1] >> 8) & 0xFF);
	}

	{
		command[0] = 58;

		iap_entry(command, result);

		printf("  uid = %X %X %X %X\r\n", result[1], result[2], result[3], result[4]);
	}
}

REGISTER_COMMAND_EX(mcu-info, mcu_info, {
	.func = mcu_info,
	.help = "Show infos about the mcu"
});

} // namespace LPC11xx
} // namespace TheFirmware
