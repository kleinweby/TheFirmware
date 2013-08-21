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

#pragma once

#include "Firmware/I2C.h"

#include <stdint.h>
#include <stddef.h>

namespace TheFirmware {
namespace Devices {

constexpr uint8_t MCP9800Address = 0x90;

class MCP9800 {
	struct {
		bool oneShot;
	};

	/// Cached config register
	uint8_t config;

	/// Reads the config register from the device
	uint8_t readConfigRegister();

	/// Writes the config register to the device
	void writeConfigRegister(uint8_t config);

public:
	/// Initializes the MCP9800
	///
	void Init();

	/// Enables the MCP9800
	///
	/// @note When configured in one shot mode, you don't need to enable
	///       the device before reading an current temperature
	/// @return True if successfull
	///
	bool enable();

	/// Shuts down the MCP9800
	///
	/// @return True if successfull
	///
	bool disable();

	/// Returns the resolution set in bits
	///
	/// @return resolution
	///
	uint8_t getResolution();

	/// Set the resolution in bits
	///
	/// @param resolution Resolution, valid range: 9-12bit
	///
	void setResolution(uint8_t resolution);

	/// Returns whetere the device is configure for one shot mode
	///
	/// @see setOneShot
	///
	bool getOneShot();

	/// Configures the one shot mode
	///
	/// @param oneShot When oneShot is enabled, the device is put in shutdown
	///                and upon readTemperature one conversation is done and returned.
	void setOneShot(bool oneShot);

	/// Reads the temperature from the devices
	///
	/// @note when in oneShot mode this methods waits for the conversion to be complete.
	uint16_t readTemperature();
};

} // namespace Devices
} // namespace TheFirmware
