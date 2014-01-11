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

#include "uart.h"

#include <mcu.h>
#include <assert.h>
#include <stdio.h>

struct uart_dev {
	struct mem_dev mem_dev;
};

bool uart_dev_read16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t* temp) {
	return false;
}

bool uart_dev_read32(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t* temp) {
	return false;
}

bool uart_dev_write16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t temp) {
	return true;
}

bool uart_dev_write32(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t temp) {

	switch(addr) {
		case 0x0:
		{
			char c = temp & 0xFF;
			putchar(c);
			break;
		}
	}

	return true;
}

uart_dev_t uart_dev_create()
{
	uart_dev_t dev = calloc(1, sizeof(struct uart_dev));

	if (!dev) {
		perror("Could not allocate uart_dev structure");
		return NULL;
	}

	dev->mem_dev.class = mem_class_io;
	dev->mem_dev.type = uart_mem_type;
	dev->mem_dev.fetch16 = uart_dev_read16;
	dev->mem_dev.fetch32 = uart_dev_read32;
	dev->mem_dev.write16 = uart_dev_write16;
	dev->mem_dev.write32 = uart_dev_write32;
	dev->mem_dev.length = 0x40;

	return dev;
}
