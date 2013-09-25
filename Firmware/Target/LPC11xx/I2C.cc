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

#include "I2C.h"

#include "LPC11xx.h"
#include "Firmware/Console/Log.h"

using namespace TheFirmware::Console;

namespace TheFirmware {
namespace LPC11xx {

enum CONSET {
	kCONSET_I2EN = 1 << 6,
	kCONSET_AA = 1 << 2,
	kCONSET_SI = 1 << 3,
	kCONSET_STO = 1 << 4,
	kCONSET_STA = 1 << 5,
};

enum CONCLR {
	kCONCLR_ACC = 1 << 2,
 	kCONCLR_SIC = 1 << 3,
	kCONCLR_STAC = 1 << 5,
	kCONCLR_I2ENC = 1 << 6,
};

// SCL Duty Cycle High
constexpr uint32_t kSCLH = 0x180;
// SCL Duty Cycle Low
constexpr uint32_t kSCLL = 0x180;

/// Global I2C Controller
class I2C I2C;

extern "C" void I2C_IRQHandler(void)
{
	I2C.isr();
}

void I2C::isr()
{
	uint8_t state = LPC_I2C->STAT;

	switch (state) {
	// Start condition issues
	case 0x08:
		this->writeIndex = 0;
		LPC_I2C->DAT = this->writeBuffer[this->writeIndex++];
		LPC_I2C->CONCLR = (kCONCLR_SIC | kCONCLR_STAC);
		break;

	case 0x10:
		this->readIndex = 0;
		LPC_I2C->DAT = this->writeBuffer[this->writeIndex++];
		LPC_I2C->CONCLR = (kCONCLR_SIC | kCONCLR_STAC);
		break;

	// Write first byte
	case 0x18:
		if (this->writeIndex < this->writeLength)
			LPC_I2C->DAT = this->writeBuffer[this->writeIndex++];
		else
			LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;

	// Byte transmitted, send next one
	case 0x28:
		// Has data to send
		if (this->writeIndex < this->writeLength)
			LPC_I2C->DAT = this->writeBuffer[this->writeIndex++];
		// Everything was sent, want to read
		else if (this->readLength > 0) {
			// Repeated start
			LPC_I2C->CONSET = kCONSET_STA;
		}
		// Everythin was sent, nothing to read, so stop
		else {
			LPC_I2C->CONSET = kCONSET_STO;
			this->done.set();
		}
		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;


	// Master Receive, SLA_R has been sent
	case 0x40:	/*  */
		if (this->readIndex + 1 < this->readLength)
			// ACK -> state 0x50
	 		LPC_I2C->CONSET = kCONSET_AA;
		else
	  		// NACK -> state 0x58
	  		LPC_I2C->CONCLR = kCONCLR_ACC;

		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;

	// Byte received
	case 0x50:
		this->readBuffer[this->readIndex++] = LPC_I2C->DAT;

		if (this->readIndex + 1 < this->readLength)
			// ACK -> state 0x50
	 		LPC_I2C->CONSET = kCONSET_AA;
		else
	  		// NACK -> state 0x58
	  		LPC_I2C->CONCLR = kCONCLR_ACC;

		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;
	
	case 0x58:
		this->readBuffer[this->readIndex++] = LPC_I2C->DAT;
		this->done.set();
		LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;

	// NACK for address (write)
	case 0x20:
	// NACK for adress (read)
	case 0x48:
		// Set stop flag
		LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		this->done.set();
		LogInfo("Stop %x %x", this->writeBuffer[0], this->writeBuffer[1]);
		break;
	default:
		LogError("Unhandled I2C state: %x %u", this->writeBuffer[0], state);
		LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		this->done.set();
	}
}

bool I2C::enable()
{
	this->lock.lock();

	// TODO: MEANING?
	LPC_SYSCON->PRESETCTRL |= (0x1<<1);

	// Enable I2C hardware clock
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);

	// Configure Pins
	LPC_IOCON->PIO0_4 &= ~0x3F; // Clear
  	LPC_IOCON->PIO0_4 |= 0x01; // SCL
 	LPC_IOCON->PIO0_5 &= ~0x3F;	// Clear
  	LPC_IOCON->PIO0_5 |= 0x01; // SCL

  	// Clear flags
  	LPC_I2C->CONCLR = kCONCLR_ACC | kCONCLR_SIC | kCONCLR_STAC | kCONCLR_I2ENC; 

  	// Reset 
  	LPC_I2C->SCLL   = kSCLL;
 	LPC_I2C->SCLH   = kSCLH;

 	// Enable interrupt
 	NVIC_EnableIRQ(I2C_IRQn);

 	// Enable hardware
	LPC_I2C->CONSET = kCONSET_I2EN;

	this->lock.unlock();
	return true;
}

void I2C::disable()
{
	LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<5);
	LPC_I2C->CONCLR = kCONCLR_I2ENC;
}

bool I2C::send(uint8_t* writeBuffer, uint32_t writeLength, uint8_t* readBuffer, uint32_t readLength)
{
	this->lock.lock();

	// Save buffers to access them in the isr
	this->writeBuffer = writeBuffer;
	this->writeIndex = 0;
	this->writeLength = writeLength;
	this->readBuffer = readBuffer;
	this->readIndex = 0;
	this->readLength = readLength;

	this->done.clear();
	// Issue start condition
	LPC_I2C->CONSET = kCONSET_STA;
	this->done.wait();

	this->lock.unlock();
	return true;
}

} // namespace LPC11xx
} // namespace TheFirmware
