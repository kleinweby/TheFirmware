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

#include "Firmware/Devices/MCP9800.h"

namespace TheFirmware {
namespace Devices {

uint8_t MCP9800::readConfigRegister()
{
	uint8_t buffer[3];
	uint8_t config = 0x00;

	// Set register pointer to 0x1 (config register)
	buffer[0] = MCP9800Address;
	buffer[1] = 0x1;

	// Read register
	buffer[2] = MCP9800Address | 0x1;

	I2C.send(buffer, 2 /* excluding the second address write */, &config, 1);

	return config;
}

void MCP9800::writeConfigRegister(uint8_t config)
{
	uint8_t buffer[4];
	
	// Set register pointer to 0x1 (config register)
	buffer[0] = MCP9800Address;
	buffer[1] = 0x1;
	buffer[2] = config;

	I2C.send(buffer, 3, NULL, 0);
}

bool MCP9800::Enable()
{
	uint8_t config = this->readConfigRegister();

	// Clear the shutdown flag
	config &= ~(1 << 0);

	this->writeConfigRegister(config);

	return true;
}

bool MCP9800::Disable()
{
	uint8_t config = this->readConfigRegister();

	// Set the shutdown flag
	config |= (1 << 0);

	this->writeConfigRegister(config);

	return true;
}

uint8_t MCP9800::getResolution()
{
	uint8_t config = this->readConfigRegister();

	return ((config >> 5) & 0x3) + 9;
}

void MCP9800::setResolution(uint8_t resolution)
{
	uint8_t config = this->readConfigRegister();

	// Clip resolution to valid range
	if (resolution < 9)
		resolution = 9;
	else if (resolution > 12)
		resolution = 12;

	// Clear resolution
	config &= ~(0x60);
	config |= (resolution - 9) << 5;

	this->writeConfigRegister(config);
}

bool MCP9800::getOneShot()
{
	uint8_t config = this->readConfigRegister();

	return (config & 0x80) == 0x80;
}

void MCP9800::setOneShot(bool oneShot)
{
	// Enabling oneshot is a two way process
	//
	// First you need to shutdown the device
	// then set the oneShot flag.
	//
	// Disabling does not depend on this specify ordering
	//
	if (oneShot)
		this->Disable();
	else
		this->Enable();

	uint8_t config = this->readConfigRegister();

	if (oneShot)
		config |= 0x80;
	else
		config &= ~0x80;

	this->writeConfigRegister(config);
}

uint16_t MCP9800::readTemperature()
{
	uint8_t buffer[3];
	uint8_t temperature[2] = {0x00, 0x00};

	// Set register pointer to 0x0 (ambient temperature)
	buffer[0] = MCP9800Address;
	buffer[1] = 0x0;

	// Read register
	buffer[2] = MCP9800Address | 0x1;

	I2C.send(buffer, 2 /* excluding the second address write */, temperature, 2);

	return temperature[0] << 8 | temperature[1];
}

} // namespace Devices
} // namespace TheFirmware
