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

#include <console.h>
#include <string.h>
#include <scheduler.h>

#include <can.h>

int can_cmd(int argc, const char** argv) {
	if (argc < 2) {
		printf("To few arguments\r\n");
		return -1;
	}

	if (strcmp(argv[1], "init") == 0) {
		printf("Initialize can...");
		if (can_init(125000)) {
			printf("done.\r\n");
		}
		else {
			printf("faild.\r\n");
		}

		return 0;
	}
	else if (strcmp(argv[1], "reset") == 0) {
		printf("Resetting can...\r\n");
		can_reset(125000);

		return 0;
	}
	else if (strcmp(argv[1], "bind") == 0) {
		printf("bind can...");
		can_bind(0x0);
		printf("done.\r\n");

		return 0;
	}
	else if (strcmp(argv[1], "send") == 0) {
		printf("send can...");
		can_send();
		printf("done.\r\n");

		return 0;
	}
	else if (strcmp(argv[1], "load") == 0) {
		while (1) {
			can_send();
			// delay(25);
		}
	}

	return -1;
}
