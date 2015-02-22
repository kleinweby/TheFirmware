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

#include <device/24xx64.h>
#include <malloc.h>
#include <string.h>
#include <scheduler.h>
#include <log.h>

struct mcp24xx64 {
	struct eeprom eeprom;
	i2c_dev_t* i2c;
};

typedef struct mcp24xx64* mcp24xx64_t;

static const uint8_t kMCP24XX64Address = 0xA0;
static const uint8_t kPageSize = 32;

static status_t mcp24xx64_read(eeprom_t _dev, uint16_t address, uint8_t* buffer, size_t bufferLength)
{
	mcp24xx64_t dev = (mcp24xx64_t)_dev;

	for (int32_t remainingLength = bufferLength; remainingLength > 0; remainingLength -= kPageSize, address += kPageSize) {
		size_t length = remainingLength < kPageSize ? remainingLength : kPageSize;
		uint8_t buf[2];

		buf[0] = (address >> 8) & 0xFF;
		buf[1] = (address >> 0) & 0xFF;

		status_t status;

		status = i2c_dev_transfer(dev->i2c, kMCP24XX64Address, buf, 2, buffer, length);
		
		if (status != STATUS_OK)
			return status;

		buffer += length;
	}

	return STATUS_OK;
}

static status_t mcp24xx64_write(eeprom_t _dev, uint16_t address, uint8_t* buffer, size_t bufferLength)
{
	mcp24xx64_t dev = (mcp24xx64_t)_dev;

	for (int32_t remainingLength = bufferLength; remainingLength > 0; remainingLength -= kPageSize, address += kPageSize) {
		size_t length = remainingLength < kPageSize ? remainingLength : kPageSize;
		uint8_t buf[2 + length];

		buf[0] = (address >> 8) & 0xFF;
		buf[1] = (address >> 0) & 0xFF;

		memcpy(&buf[2], buffer, length);

		status_t status;

		do {
			status = i2c_dev_transfer(dev->i2c, kMCP24XX64Address, buf, 2 + length, NULL, 0);
		} while (status != STATUS_OK);

		buffer += length;
	}

	return STATUS_OK;
}

eeprom_t mcp24xx64_create(i2c_dev_t* i2c)
{
	mcp24xx64_t dev = malloc(sizeof(struct mcp24xx64));

	if (!dev) {
		return NULL;
	}

	dev->i2c = i2c;
	dev->eeprom.write = mcp24xx64_write;
	dev->eeprom.read = mcp24xx64_read;

	return (eeprom_t)dev;
}


