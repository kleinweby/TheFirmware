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

#include "Firmware/Schedule/Flag.h"
#include "Firmware/Schedule/Mutex.h"

#include <stdint.h>

namespace TheFirmware {
namespace LPC11xx {

/// The I2C module overrides the I2C IRQ Handler
extern "C" void I2C_IRQHandler(void);

class I2C {
	uint8_t* writeBuffer;
	uint32_t writeIndex;
	uint32_t writeLength;
	uint8_t* readBuffer;
	uint32_t readIndex;
	uint32_t readLength;
	Schedule::Flag done;
	Schedule::Mutex lock;

	/// Called by the I2C interrupt to handle it
	void isr();

	friend void I2C_IRQHandler(void);
public:

	/// Enables the I2C hardware
	///
	/// @return true on success, false otherwise
	bool enable();

	/// Disables the I2C hardware
	///
	void disable();

	/// Send data over I2C
	///
	/// 
	///
	/// 
	bool send(uint8_t* writeBuffer, uint32_t writeLength, uint8_t* readBuffer, uint32_t readLength);
};

extern I2C I2C;

} // namespace LPC11xx
} // namespace TheFirmware
