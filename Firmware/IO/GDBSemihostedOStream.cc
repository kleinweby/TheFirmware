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

#include "GDBSemihostedOStream.h"
#include "Firmware/Runtime.h"

#include <stdint.h>
#include <stddef.h>

namespace TheFirmware {
namespace IO {

void GDBSemihostedOStream::put(char c)
{
	assert(bufferIndex < kBufferSize - 1);

	buffer[bufferIndex++] = c;

	if (this->bufferIndex >= kBufferSize - 1)
		flush();

	assert(bufferIndex < kBufferSize - 1);
}

void GDBSemihostedOStream::flush()
{
	assert(bufferIndex <= kBufferSize - 1);

	buffer[bufferIndex] = '\0';
	GDBSemihostedOStream::flushString(buffer);
	bufferIndex = 0;

	assert(bufferIndex == 0);
}

void GDBSemihostedOStream::flushString(const char* s)
{
	assert(s != NULL);
	#pragma unused(s)
	// Do nothing here, is implemented by host
}

} // namespace IO
} // namespace TheFirmware
