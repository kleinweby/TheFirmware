//
// Copyright (c) 2014, Christian Speich
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, global_dev list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, global_dev list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// global_dev SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF global_dev
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <peripheral/i2c_dev.h>

#include <malloc.h>
#include <log.h>
#include <semaphore.h>
#include <runtime.h>
#include <irq.h>

#include "LPC11xx.h"

struct i2c_dev {
	struct semaphore lock;

	// Current transfer
	struct semaphore done;
	i2c_addr_t addr;
	const uint8_t* writeBuffer;
	size_t writeBufferIndex;
	size_t writeBufferLength;
	uint8_t* readBuffer;
	size_t readBufferIndex;
	size_t readBufferLength;
};

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
static const uint32_t kSCLH = 0x180;
// SCL Duty Cycle Low
static const uint32_t kSCLL = 0x180;

static i2c_dev_t global_dev;

static void i2c_dev_isr(void)
{
	uint8_t state = LPC_I2C->STAT;

	switch (state) {
	// Start condition issues
	case 0x08:
		global_dev->writeBufferIndex = 0;
		LPC_I2C->DAT = global_dev->addr;
		LPC_I2C->CONCLR = (kCONCLR_SIC | kCONCLR_STAC);
		break;

	case 0x10:
		global_dev->readBufferIndex = 0;
		LPC_I2C->DAT = global_dev->addr | 0x1;
		LPC_I2C->CONCLR = (kCONCLR_SIC | kCONCLR_STAC);
		break;

	// Write first byte
	case 0x18:
		if (global_dev->writeBufferIndex < global_dev->writeBufferLength)
			LPC_I2C->DAT = global_dev->writeBuffer[global_dev->writeBufferIndex++];
		else
			LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;

	// Byte transmitted, send next one
	case 0x28:
		// Has data to send
		if (global_dev->writeBufferIndex < global_dev->writeBufferLength)
			LPC_I2C->DAT = global_dev->writeBuffer[global_dev->writeBufferIndex++];
		// Everything was sent, want to read
		else if (global_dev->readBufferLength > 0) {
			// Repeated start
			LPC_I2C->CONSET = kCONSET_STA;
		}
		// Everythin was sent, nothing to read, so stop
		else {
			LPC_I2C->CONSET = kCONSET_STO;
			semaphore_signal(&global_dev->done);
		}
		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;


	// Master Receive, SLA_R has been sent
	case 0x40:	/*  */
		if (global_dev->readBufferIndex + 1 < global_dev->readBufferLength)
			// ACK -> state 0x50
	 		LPC_I2C->CONSET = kCONSET_AA;
		else
	  		// NACK -> state 0x58
	  		LPC_I2C->CONCLR = kCONCLR_ACC;

		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;

	// Byte received
	case 0x50:
		global_dev->readBuffer[global_dev->readBufferIndex++] = LPC_I2C->DAT;

		if (global_dev->readBufferIndex + 1 < global_dev->readBufferLength)
			// ACK -> state 0x50
	 		LPC_I2C->CONSET = kCONSET_AA;
		else
	  		// NACK -> state 0x58
	  		LPC_I2C->CONCLR = kCONCLR_ACC;

		LPC_I2C->CONCLR = kCONCLR_SIC;
		break;
	
	case 0x58:
		global_dev->readBuffer[global_dev->readBufferIndex++] = LPC_I2C->DAT;
		semaphore_signal(&global_dev->done);
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
		semaphore_signal(&global_dev->done);
		log(LOG_LEVEL_INFO, "Stop %x %x", global_dev->writeBuffer[0], global_dev->writeBuffer[1]);
		break;
	default:
		log(LOG_LEVEL_ERROR, "Unhandled I2C state: %x %u", global_dev->writeBuffer[0], state);
		LPC_I2C->CONSET = kCONSET_STO;
		LPC_I2C->CONCLR = kCONCLR_SIC;
		semaphore_signal(&global_dev->done);
	}
}

i2c_dev_t i2c_create_dev()
{
	i2c_dev_t dev = malloc_raw(sizeof(struct i2c_dev));

	if (!dev)
		return NULL;

	semaphore_init(&dev->lock, 1);
	semaphore_init(&dev->done, 0);

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

	global_dev = dev;

	assert(irq_register(IRQ15, i2c_dev_isr), "Could not register i2c irq");
	irq_enable(IRQ15);

 	// Enable hardware
	LPC_I2C->CONSET = kCONSET_I2EN;

	return dev;
}

bool i2c_dev_transfer(i2c_dev_t dev, i2c_addr_t addr, const uint8_t* writeBuffer, size_t writeBufferLength, uint8_t* readBuffer, size_t readBufferLength)
{
	semaphore_wait(&dev->lock);
	// Configure transfer
	dev->addr = addr;
	dev->writeBuffer = writeBuffer;
	dev->writeBufferLength = writeBufferLength;
	dev->readBuffer = readBuffer;
	dev->readBufferLength = readBufferLength;

	// Start transfer
	LPC_I2C->CONSET = kCONSET_STA;

	// Wait for transfer to complete
	semaphore_wait(&dev->done);
	semaphore_signal(&dev->lock);
	return false;
}
