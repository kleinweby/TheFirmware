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

#include "Firmware/Runtime.h"

#include "Firmware/Console/string.h"
#include "Firmware/FirmwareInfo.h"

namespace TheFirmware {
namespace Console {

LINKER_SYMBOL(CLICommands, CLICommand*);
LINKER_SYMBOL(CLICommandsLength, uint32_t);

void run(const char* command, int argc, char** argv)
{
	uint32_t count = CLICommandsLength/sizeof(CLICommand);

	if (command == NULL)
		return;

	for (uint32_t i = 0; i < count; i++) {
		int result = strcmp(command, CLICommands[i].name);
		
		if (result == 0) {
			CLICommands[i].info.func(argc, argv);
			break;
		}
		else if (result < 0) {
			printf("Command not found!\r\n");
			break;
		}
	}
}

void version(int argc, char** argv)
{
	printf("TheOS %s\r\n\r\n", FirmwareVersion);

	if (FirmwareGitVersion || FirmwareGitBranch) {
		printf(" Build information:\r\n");
		printf("  date: %s\r\n", FirmwareBuildDate);
		printf("  rev: %s\r\n", FirmwareGitVersion);
		printf("  branch: %s\r\n", FirmwareGitBranch);
	}
}

REGISTER_COMMAND(version, {
	.func = version,
	.help = "Show version of TheFirmware"
});

void help(int argc, char** argv)
{
	uint32_t count = CLICommandsLength/sizeof(CLICommand);

	printf("Avaiable commands:\r\n");
	for (uint32_t i = 0; i < count; i++) {
		printf("    %s - %s\r\n", CLICommands[i].name, CLICommands[i].info.help);
	}
}

REGISTER_COMMAND(help, {
	.func = help,
	.help = "Prints this help"
});

} // namespace Console
} // namespace TheFirmware
