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

#include "ram.h"

#include <mcu.h>
#include <assert.h>
#include <stdio.h>

struct ram_dev {
	struct mem_dev mem_dev;

	uint32_t* ram;
};

bool ram_dev_read16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t* temp) {
	*temp = ((uint16_t*)((ram_dev_t)mem_dev)->ram)[addr >> 1];

	return true;
}

bool ram_dev_read32(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t* temp) {
	*temp = (((ram_dev_t)mem_dev)->ram)[addr >> 2];

	return true;
}

bool ram_dev_write16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t temp) {
	((uint16_t*)((ram_dev_t)mem_dev)->ram)[addr >> 1] = temp;

	return true;
}

bool ram_dev_write32(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t temp) {
	(((ram_dev_t)mem_dev)->ram)[addr >> 2] = temp;

	return true;
}

ram_dev_t ram_dev_create(size_t size)
{
	assert((size & 0x3FF) == 0 && "Ram size must be multiple of 1024");

	ram_dev_t dev = calloc(1, sizeof(struct ram_dev));

	if (!dev) {
		perror("Could not allocate ram_dev structure");
		return NULL;
	}

	dev->mem_dev.type = ram_mem_type;
	dev->mem_dev.fetch16 = ram_dev_read16;
	dev->mem_dev.fetch32 = ram_dev_read32;
	dev->mem_dev.write16 = ram_dev_write16;
	dev->mem_dev.write32 = ram_dev_write32;
	dev->mem_dev.length = size;
	dev->ram = malloc(size);

	if (!dev->ram) {
		perror("Could not allocate ram");
		free(dev);
		return NULL;
	}

	return dev;
}
