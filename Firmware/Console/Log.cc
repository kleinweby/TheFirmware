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

#include "Log.h"
#include "Firmware/Console/Console.h"
#include "Firmware/Runtime.h"

#include <stdint.h>

namespace TheFirmware {
namespace Console {

void Logv(LogLevel logLevel, const char* format, va_list args)
{
	assert(format != NULL);
	
	// Print level
	switch(logLevel) {
		case kLogLevelError:
			printf("\033[1;31m[E]\033[0m ");
			break;
		case kLogLevelWarn:
			printf("\033[1;33m[W]\033[0m ");
			break;
		case kLogLevelInfo:
			printf("\033[0;34m[I]\033[0m ");
			break;
		case kLogLevelVerbose:
			printf("\033[0;32m[V]\033[0m ");
			break;
		case kLogLevelDebug:
			printf("\033[0;32m[D]\033[0m ");
			break;
	}

	printf_va(format, args);

	printf("\r\n");
}

} // namespace Console
} // namespace TheFirmware
