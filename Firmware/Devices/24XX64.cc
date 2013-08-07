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

#include "Firmware/Devices/24XX64.h"
#include "Firmware/Runtime.h"

namespace TheFirmware {
namespace Devices {

bool MCP24XX64::write(uint16_t address, uint8_t* buffer, uint16_t bufferLength)
{
	assert(bufferLength <= 31);
	uint8_t writeBuffer[3 /* device addr & mem addr */ + 31 /* page size */];

	// Write address to read
	writeBuffer[0] = MCP24XX64Address;
	writeBuffer[1] = (address >> 8) & 0xFF;
	writeBuffer[2] = (address >> 0) & 0xFF;

	// Data
	for (uint8_t i = 3; i < bufferLength + 3; i++)
		writeBuffer[i] = buffer[i - 3];

	I2C.send(writeBuffer, 3 + bufferLength, NULL, 0);

	return true;
}

bool MCP24XX64::read(uint16_t address, uint8_t* buffer, uint16_t bufferLength)
{
	uint8_t writeBuffer[4];

	// Write address to read
	writeBuffer[0] = MCP24XX64Address;
	writeBuffer[1] = (address >> 8) & 0xFF;
	writeBuffer[2] = (address >> 0) & 0xFF;

	// Read register
	writeBuffer[3] = MCP24XX64Address | 0x1;

	I2C.send(writeBuffer, 3 /* excluding the second address write */, buffer, bufferLength);

	return true;
}

}
}
