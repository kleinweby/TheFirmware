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

//
// Much inspiration from github.com/dwelch67/thumbulator
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef enum {
	reg_sp = 13,
	reg_pc = 15,
	reg_count
} reg_t;

typedef struct mcu* mcu_t;
typedef struct mem_dev* mem_dev_t;
bool mcu_add_mem_dev(mcu_t mcu, uint32_t offset, mem_dev_t dev);

typedef struct ram_dev* ram_dev_t;
ram_dev_t ram_dev_create(size_t size);

struct mem_dev {
	uint16_t offset;
	uint16_t length;
	mem_dev_t next;

	bool (*fetch16)(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t* valueOut);
	bool (*fetch32)(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t* valueOut);

	bool (*write16)(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t value);
	bool (*write32)(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint32_t value);
};

struct mcu {
	mem_dev_t mem_devs;

	uint32_t regs[reg_count];
};

mcu_t mcu_cortex_m0p_create(size_t ramsize)
{
	mcu_t mcu = calloc(1, sizeof(struct mcu));

	if (!mcu) {
		perror("Could not allocate mcu struct");
		return NULL;
	}

	{
		ram_dev_t ram = ram_dev_create(ramsize);

		if (!ram) {
			printf("Could not allocate ram_dev");
			return NULL;
		}

		if (!mcu_add_mem_dev(mcu, 0x20000000, (mem_dev_t)ram)) {
			printf("Could not add ram_dev to mcu");
			return NULL;
		}
	}

	return mcu;
}

#define DECLARE_MEM_OP(name, type) \
bool mcu_##name(mcu_t mcu, uint32_t addr, type value) \
{ \
	for (mem_dev_t dev = mcu->mem_devs;  \
		dev != NULL; dev = dev->next) { \
		if (dev->offset <= addr && addr < dev->offset + dev->length) {\
			if (dev->name) { \
				return dev->name(mcu, dev, addr - dev->offset, value); \
			} \
			else { \
				return false; \
			} \
		} \
	} \
	return false; \
}

// DECLARE_MEM_OP(fetch16, uint16_t*);
bool mcu_fetch16(mcu_t mcu, uint32_t addr, uint16_t* value)
{
	for (mem_dev_t dev = mcu->mem_devs; 
		dev != NULL; dev = dev->next) { 
		if (dev->offset <= addr && addr < dev->offset + dev->length) {
			if (dev->fetch16) { 
				return dev->fetch16(mcu, dev, addr - dev->offset, value); 
			} 
			else { 
				return false; 
			} 
		} 
	} 
	return false; 
}
DECLARE_MEM_OP(fetch32, uint32_t*);
DECLARE_MEM_OP(write16, uint16_t);
DECLARE_MEM_OP(write32, uint32_t);

bool mcu_add_mem_dev(mcu_t mcu, uint32_t offset, mem_dev_t dev) 
{
	// Quick and dirty, unordered

	dev->next = mcu->mem_devs;
	dev->offset = 0;//offset;
	mcu->mem_devs = dev;

	return true;
}

uint32_t mcu_read_reg(mcu_t mcu, reg_t reg)
{
	return mcu->regs[reg];
}

void mcu_write_reg(mcu_t mcu, reg_t reg, uint32_t val)
{
	mcu->regs[reg] = val;
}

void mcu_update_nflag(mcu_t mcu, uint32_t c)
{

}

void mcu_update_zflag(mcu_t mcu, uint32_t c)
{
	
}

void mcu_update_cflag(mcu_t mcu, uint32_t a, uint32_t b, uint32_t c)
{
	
}

void mcu_update_vflag(mcu_t mcu, uint32_t a, uint32_t b, uint32_t c)
{
	
}

struct mcu_instr_def {
	uint16_t mask;
	uint16_t instr;
	bool (^impl)(mcu_t mcu, uint16_t instr);
} instrs[] = {
	// TODO: ADC

	//ADD(1) small immediate two registers
	{
		.mask = 0xFE00,
		.instr = 0x1C00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint8_t dest = (instr >> 0) & 0x7;
        	uint8_t src  = (instr >> 3) & 0x7;
        	uint8_t b  = (instr >> 6) & 0x7;

        	uint32_t a = mcu_read_reg(mcu, src);
        	uint32_t c = a + b;

        	mcu_write_reg(mcu, dest, c);
            mcu_update_nflag(mcu, c);
            mcu_update_zflag(mcu, c);
            mcu_update_cflag(mcu, a, b, 0);
            mcu_update_vflag(mcu, a, b, 0);

			return true;
		}
	},

	//ADD(2) big immediate one register
	{
		.mask = 0xF800,
		.instr = 0x3000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint8_t reg = (instr >> 8) & 0x7;
        	uint8_t b  = (instr >> 0) & 0xFF;

        	uint32_t a = mcu_read_reg(mcu, reg);
        	uint32_t c = a + b;

        	mcu_write_reg(mcu, reg, c);
            mcu_update_nflag(mcu, c);
            mcu_update_zflag(mcu, c);
            mcu_update_cflag(mcu, a, b, 0);
            mcu_update_vflag(mcu, a, b, 0);

			return true;
		}
	},

	//ADD(3) three registers
	{
		.mask = 0xFE00,
		.instr = 0x1800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint8_t src = (instr >> 0) & 0x7;
        	uint8_t dest  = (instr >> 3) & 0x7;
        	uint8_t src2  = (instr >> 6) & 0x7;

        	uint32_t a = mcu_read_reg(mcu, src);
        	uint32_t b = mcu_read_reg(mcu, src2);
        	uint32_t c = a + b;

        	mcu_write_reg(mcu, dest, c);
            mcu_update_nflag(mcu, c);
            mcu_update_zflag(mcu, c);
            mcu_update_cflag(mcu, a, b, 0);
            mcu_update_vflag(mcu, a, b, 0);

			return true;
		}
	},

	//ADD(4) two registers one or both high no flags
	{
		.mask = 0xFF00,
		.instr = 0x4400,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			if (( instr >> 6 ) & 3) {
				printf("UNPREDICTABLE");
			}

			reg_t reg = ((instr >> 0) & 0x7) | ((instr >> 4) & 0x8);
        	reg_t src2  = (instr >> 3) & 0xF;

        	uint32_t a = mcu_read_reg(mcu, reg);
        	uint32_t b = mcu_read_reg(mcu, src2);
        	uint32_t c = a + b;

        	if (reg == reg_pc) {
        		if ((c & 1) == 0) {
        			printf("add pc,... produced arm address, arm mode not supported!\n");
        			return false;
        		}

        		c &= ~1; // Is this needed? c&1 would catch all cases wouldn't it?
        		c += 2; // PC is special
        	}

        	mcu_write_reg(mcu, reg, c);

			return true;
		}
	},

	//ADD(5) rd = pc plus immediate
	{
		.mask = 0xF800,
		.instr = 0xA000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 8) & 0x7;
			uint32_t imm = ((instr >> 0) & 0xFF) << 2;
        	uint32_t a = mcu_read_reg(mcu, reg_pc);
        	uint32_t c = (a & (~3) ) + imm;

        	mcu_write_reg(mcu, dest, c);

			return true;
		}
	},

	//ADD(6) rd = sp plus immediate
	{
		.mask = 0xF800,
		.instr = 0xA800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 8) & 0x7;
			uint32_t imm = ((instr >> 0) & 0xFF) << 2;
        	uint32_t a = mcu_read_reg(mcu, reg_sp);
        	uint32_t c = a + imm;

        	mcu_write_reg(mcu, dest, c);

			return true;
		}
	},

	//ADD(7) sp plus immediate
	{
		.mask = 0xFF80,
		.instr = 0xB000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = ((instr >> 0) & 0x7F) << 2;
        	uint32_t a = mcu_read_reg(mcu, reg_sp);
        	uint32_t c = a + imm;

        	mcu_write_reg(mcu, reg_sp, c);

			return true;
		}
	},

	//AND
	{
		.mask = 0xFFC0,
		.instr = 0x4000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

        	uint32_t a = mcu_read_reg(mcu, reg);
        	uint32_t b = mcu_read_reg(mcu, src2);
        	uint32_t c = a & b;

        	mcu_write_reg(mcu, reg, c);
        	mcu_update_nflag(mcu, c);
        	mcu_update_zflag(mcu, c);

			return true;
		}
	},
	{ 0, 0, NULL }
};

bool mcu_instr_step(mcu_t mcu)
{
	uint32_t pc = mcu_read_reg(mcu, reg_pc);
	uint16_t instr;

	if (!mcu_fetch16(mcu, pc, &instr)) {
		printf("ERROR: could not fetch instruction. [pc=%x]", pc);
		return false;
	}

	mcu_write_reg(mcu, reg_pc, pc+2);

	for (struct mcu_instr_def* def = instrs; def->impl != NULL; ++def) {
		if ((instr & def->mask) == def->instr) {
			return def->impl(mcu, instr);
		}
	}

	return false;
}

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

int main(int argc, char** argv) {
	mcu_t mcu = mcu_cortex_m0p_create(16 * 1024);
	
	mcu_write32(mcu, 0x0, 0xAABBCCDD);

	if (mcu_instr_step(mcu)) {
		printf("step\n");
	}
	else {
		printf("no step\n");
	}

	printf("Bla");

	return 0;
}

