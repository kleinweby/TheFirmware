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

#include <mcu.h>

#include <stdio.h>
#include <ram.h>
#include <flash.h>
#include <uart.h>
#include <unittest.h>

typedef struct mcu_cortex_m0p* mcu_cortex_m0p_t;

extern struct mcu_instr16 mcu_instr16_cortex_m0p[];
extern struct mcu_instr32 mcu_instr32_cortex_m0p[];

enum {
	CPSR_N = (1<<31),
	CPSR_Z = (1<<30),
	CPSR_C = (1<<29),
	CPSR_V = (1<<28),
	CPSR_Q = (1<<27)
};

mcu_t mcu_cortex_m0p_create(struct ev_loop *loop, size_t ramsize)
{
	mcu_cortex_m0p_t mcu = calloc(1, sizeof(struct mcu_cortex_m0p));

	if (!mcu) {
		perror("Could not allocate mcu struct");
		return NULL;
	}

	mcu_init((mcu_t)mcu, loop);
	mcu->mcu.instrs16 = mcu_instr16_cortex_m0p;
	mcu->mcu.instrs32 = mcu_instr32_cortex_m0p;

	{
		flash_dev_t flash = flash_dev_create(32 * 1024);

		if (!flash) {
			printf("Could not allocate flash_dev");
			return NULL;
		}

		if (!mcu_add_mem_dev((mcu_t)mcu, 0x0, (mem_dev_t)flash)) {
			printf("Could not add ram_dev to flash");
			return NULL;
		}
	}

	{
		ram_dev_t ram = ram_dev_create(ramsize);

		if (!ram) {
			printf("Could not allocate ram_dev");
			return NULL;
		}

		if (!mcu_add_mem_dev((mcu_t)mcu, 0x10000000, (mem_dev_t)ram)) {
			printf("Could not add ram_dev to mcu");
			return NULL;
		}
	}

	{
		uart_dev_t uart = uart_dev_create();

		if (!uart) {
			printf("Could not create uart_dev");
			return NULL;
		}

		if (!mcu_add_mem_dev((mcu_t)mcu, 0xE0000000, (mem_dev_t)uart)) {
			printf("Could not add uart_dev to mcu");
			return NULL;
		}
	}

	{
		unittest_dev_t unittest = unittest_dev_create();

		if (!unittest) {
			printf("Could not create unittest_dev");
			return NULL;
		}

		if (!mcu_add_mem_dev((mcu_t)mcu, 0xEE000000, (mem_dev_t)unittest)) {
			printf("Could not add unittest_dev to mcu");
			return NULL;
		}
	}

	return (mcu_t)mcu;
}

bool mcu_reset(mcu_t _mcu)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	{
		uint32_t val;

		if (!mcu_fetch32(_mcu, 0x0, &val)) {
			printf("Fetch faild");
			return false;
		}

		mcu_write_reg(_mcu, REG_SP, val);
	}

	{
		uint32_t val;

		if (!mcu_fetch32(_mcu, 0x4, &val)) {
			printf("Fetch faild");
			return false;
		}

		if ((val & 1) == 0) {
			printf("Reset vector contains arm address\n");
			return false;
		}

		mcu_write_reg(_mcu, REG_PC, val + 2);
	}

	mcu->processor_mode = processor_thread_mode;
	mcu_write_reg(_mcu, REG_EPSR, 1 << 24);

	return true;
}

bool mcu_do_exception(mcu_t mcu, exception_t exception)
{
	if (exception == exception_reset)
		return mcu_reset(mcu);

	return mcu_do_irq(mcu, exception - 16);
}

bool mcu_do_fault(mcu_t mcu, fault_t fault)
{
	return mcu_do_exception(mcu, exception_hardfault);
}

bool mcu_do_irq(mcu_t mcu, irq_t fault)
{
	return false;
}

uint32_t mcu_read_reg(mcu_t _mcu, reg_t reg)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	if (reg == REG_SP) {
		if (mcu->regs[reg] & 0x2)
			reg = REG_PSP;
		else
			reg = REG_MSP;
	}
	else if (reg == REG_APSR)
		return mcu->regs[REG_XPSR] & 0xF0000000;
	else if (reg == REG_IPSR)
		return mcu->regs[REG_XPSR] & 0x1F;
	else if (reg == REG_EPSR)
		return mcu->regs[REG_XPSR] & 0x1000000;

	return mcu->regs[reg];
}

void mcu_write_reg(mcu_t _mcu, reg_t reg, uint32_t val)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	if (reg == REG_PC) {
		val &= ~1;
	}

	if (reg == REG_SP) {
		if (mcu->regs[reg] & 0x2)
			reg = REG_PSP;
		else
			reg = REG_MSP;
	}

	if (reg == REG_APSR)
		mcu->regs[REG_XPSR] = (mcu->regs[REG_XPSR] & ~0xF0000000) | (val & 0xF0000000);
	else if (reg == REG_IPSR)
		mcu->regs[REG_XPSR] = (mcu->regs[REG_XPSR] & ~0x1F) | (val & 0x1F);
	else if (reg == REG_EPSR)
		mcu->regs[REG_XPSR] = (mcu->regs[REG_XPSR] & ~0x1000000) | (val & 0x1000000);
	else
		mcu->regs[reg] = val;
}

bool mcu_instr_step(mcu_t mcu)
{
	uint32_t pc = mcu_read_reg(mcu, REG_PC);
	uint32_t old_pc = pc;
	uint32_t instr;
	bool thritytwo = false;

	if (!mcu_fetch16(mcu, pc - 2, (uint16_t*)&instr)) {
		printf("ERROR: could not fetch instruction. [pc=0x%x]", pc);
		mcu_halt(mcu, HALT_HARD_FAULT);
		return false;
	}

	// 32-bit thumb instruction
	if ((instr & 0xF800) == 0xF800 ||
		(instr & 0xF800) == 0xE800 ||
		(instr & 0xF800) == 0xF000) {
		thritytwo = true;

		instr <<= 16;
		pc += 2;

		if (!mcu_fetch16(mcu, pc - 2, (uint16_t*)&instr)) {
			printf("ERROR: could not fetch instruction. [pc=0x%x]", pc);
			mcu_halt(mcu, HALT_HARD_FAULT);
			return false;
		}
	}

	mcu_write_reg(mcu, REG_PC, pc+2);

	if (thritytwo) {
		for (mcu_instr32_t def = mcu->instrs32; def->impl != NULL; ++def) {
			if ((instr & def->mask) == def->instr){
				if (!def->impl(mcu, instr)) {
					mcu_write_reg(mcu, REG_PC, old_pc);
					return false;
				}
				else
					return true;
			}
		}

		printf("Unkown 32-bit thumb instruction: 0x%08x", instr);
	}
	else {
		for (mcu_instr16_t def = mcu->instrs16; def->impl != NULL; ++def) {
			if ((instr & def->mask) == def->instr) {
				if (!def->impl(mcu, instr)) {
					mcu_write_reg(mcu, REG_PC, old_pc);
					return false;
				}
				else
					return true;
			}
		}

		printf("Unkown 16-bit thumb instruction: 0x%04x", instr);
	}

	mcu_write_reg(mcu, REG_PC, old_pc);
	mcu_halt(mcu, HALT_UNKOWN_INSTRUCTION);

	return false;
}

static void mcu_update_nflag(void* _mcu, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_APSR);

	if( c & (1 << 31))
		cpsr |= CPSR_N;
	else
		cpsr &= ~CPSR_N;

	mcu_write_reg(_mcu, REG_APSR, cpsr);
}

static void mcu_update_zflag(void* _mcu, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_APSR);

	if (c == 0)
		cpsr |= CPSR_Z;
	else
		cpsr &= ~CPSR_Z;

	mcu_write_reg(_mcu, REG_APSR, cpsr);
}

static void mcu_update_cflag_bit(void* _mcu, uint32_t val)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_APSR);

	if (val)
    	cpsr |= CPSR_C;
    else
    	cpsr &= ~CPSR_C;

    mcu_write_reg(_mcu, REG_APSR, cpsr);
}

static void mcu_update_cflag(void* _mcu, uint32_t a, uint32_t b, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	uint32_t temp;

	// carry in
    temp = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c;
    //carry out
    temp = (temp >> 31) + (a>>31) + (b >> 31);
    mcu_update_cflag_bit(mcu, temp & 2);
}

static void mcu_update_vflag_bit(void* _mcu, uint32_t val)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_APSR);

	if (val)
    	cpsr |= CPSR_V;
    else
    	cpsr &= ~CPSR_V;

    mcu_write_reg(_mcu, REG_APSR, cpsr);
}

static void mcu_update_vflag(void* _mcu, uint32_t a, uint32_t b, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	uint32_t temp1;
    uint32_t temp2;

    temp1 = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
    temp1 >>= 31; //carry in in lsbit
    temp2 = (temp1 & 1) + (( a>> 31) & 1) + ((b >> 31) & 1); //carry out
    temp2 >>= 1; //carry out in lsbit
    temp1 = (temp1 ^ temp2) & 1; //if carry in != carry out then signed overflow
    mcu_update_vflag_bit(mcu, temp1);
}

static void mcu_fetch_error(void* _mcu, uint32_t addr)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	printf("Fetch error at %x\n", addr);
	mcu_halt((mcu_t)mcu, HALT_HARD_FAULT);
}

static void mcu_write_error(void* _mcu, uint32_t addr)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	printf("Write error at %x\n", addr);
	mcu_halt((mcu_t)mcu, HALT_HARD_FAULT);
}

struct mcu_instr16 mcu_instr16_cortex_m0p[] = {
	// TODO: ADC

	//ADD(1) small immediate two registers
	{
		.mask = 0xFE00,
		.instr = 0x1C00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint8_t dest = (instr >> 0) & 0x7;
        	uint8_t src  = (instr >> 3) & 0x7;
        	uint8_t b  = (instr >> 6) & 0x7;

        	trace_instr16("adds r%u,r%u,#0x%X\n", dest, src, b);

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

        	trace_instr16("adds r%u,#0x%02X\n", reg, b);

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
        	uint8_t dest  = (instr >> 0) & 0x7;
			uint8_t src = (instr >> 3) & 0x7;
        	uint8_t src2  = (instr >> 6) & 0x7;

        	trace_instr16("adds r%u,r%u,r%u\n", dest, src, src2);

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

        	trace_instr16("add r%u,r%u\n", reg, src2);

        	uint32_t a = mcu_read_reg(mcu, reg);
        	uint32_t b = mcu_read_reg(mcu, src2);
        	uint32_t c = a + b;

        	if (reg == REG_PC) {
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

			trace_instr16("add r%u,PC,#0x%02X\n", dest, imm);

        	uint32_t a = mcu_read_reg(mcu, REG_PC);
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

			trace_instr16("add r%u,SP,#0x%02X\n", dest, imm);

        	uint32_t a = mcu_read_reg(mcu, REG_SP);
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

			trace_instr16("add SP,#0x%02X\n", imm);

        	uint32_t a = mcu_read_reg(mcu, REG_SP);
        	uint32_t c = a + imm;

        	mcu_write_reg(mcu, REG_SP, c);

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

			trace_instr16("ands r%u,r%u\n", reg, src2);

        	uint32_t a = mcu_read_reg(mcu, reg);
        	uint32_t b = mcu_read_reg(mcu, src2);
        	uint32_t c = a & b;

        	mcu_write_reg(mcu, reg, c);
        	mcu_update_nflag(mcu, c);
        	mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//ASR(1) two register immediate
	{
		.mask = 0xF800,
		.instr = 0x1000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest     = (instr >> 0) & 0x7;
			reg_t src      = (instr >> 3) & 0x7;
			uint32_t shift = (instr >> 6) & 0x1F;

			trace_instr16("asrs r%u,r%u,#0x%X\n", dest, src, shift);

        	uint32_t a = mcu_read_reg(mcu, src);

        	if (shift == 0) {
        		if (a & (1 << 31)) {
        			mcu_update_cflag_bit(mcu, 1);
        			a = ~0;
        		}
        		else {
        			mcu_update_cflag_bit(mcu, 0);
        			a = 0;
        		}
        	}
        	else {
        		mcu_update_cflag_bit(mcu, a & (1 << (shift - 1)));

        		uint32_t b = a & (1 << 31);

        		a >>= shift;

        		// Sign
        		if (b)
        			a |= (~0) << (32 - shift);
        	}

        	mcu_write_reg(mcu, dest, a);
        	mcu_update_nflag(mcu, a);
        	mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//ASR(2) two register
	{
		.mask = 0xF800,
		.instr = 0x1000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg      = (instr >> 0) & 0x7;
			reg_t shift_reg = (instr >> 3) & 0x7;

			trace_instr16("asrs r%u,r%u\n", reg, shift_reg);

        	uint32_t a     = mcu_read_reg(mcu, reg);
        	uint32_t shift = mcu_read_reg(mcu, shift_reg) & 0xFF;

        	if (shift == 0) {
        		// NOP
        	}
        	else if (shift < 32) {
        		mcu_update_cflag_bit(mcu, a & (1 << (shift - 1)));

        		uint32_t b = a & (1 << 31);

        		a >>= shift;

        		// Sign
        		if (b)
        			a |= (~0) << (32 - shift);
        	}
        	else {
        		if (a & (1 << 31)) {
        			mcu_update_cflag_bit(mcu, 1);
        			a = ~0;
        		}
        		else {
        			mcu_update_cflag_bit(mcu, 0);
        			a = 0;
        		}
        	}

        	mcu_write_reg(mcu, reg, a);
        	mcu_update_nflag(mcu, a);
        	mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//B(1) conditional branch
	{
		.mask = 0xF000,
		.instr = 0xD000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t op     = (instr >> 8) & 0xF;
			uint32_t new_pc = (instr >> 0) & 0xFF;

			if (new_pc & 0x80)
				new_pc |= (~0) << 8;

			new_pc = (new_pc << 1) + mcu_read_reg(mcu, REG_PC) + 2;

			uint32_t cpsr = mcu_read_reg(mcu, REG_APSR);
			switch (op) {
				case 0x0: //b eq  z set
					trace_instr16("beq 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_Z)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x1: //b ne  z clear
                	trace_instr16("bne 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_Z))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x2: //b cs c set
                	trace_instr16("bcs 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_C)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x3: //b cc c clear
                	trace_instr16("bcc 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_C))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x4: //b mi n set
                	trace_instr16("bmi 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_N)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x5: //b pl n clear
                	trace_instr16("bpl 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_N))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x6: //b vs v set
                	trace_instr16("bvs 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_V)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x7: //b vc v clear
                	trace_instr16("bvc 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_V))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x8: //b hi c set z clear
                	trace_instr16("bhi 0x%08X\n", new_pc - 3);

                	if((cpsr & CPSR_C) && !(cpsr & CPSR_Z))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x9: //b ls c clear or z set
                	trace_instr16("bls 0x%08X\n", new_pc - 3);

                	if((cpsr & CPSR_Z) || !(cpsr & CPSR_C))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0xA: //b ge N == V
                	trace_instr16("bge 0x%08X\n", new_pc - 3);

                	if (     ((cpsr & CPSR_N)  &&  (cpsr & CPSR_V))
                		|| ((!(cpsr & CPSR_N)) && !(cpsr & CPSR_V)))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xB: //b lt N != V
                	trace_instr16("blt 0x%08X\n", new_pc - 3);

                	if (   ((!(cpsr&CPSR_N))&&(cpsr&CPSR_V))
                		|| ((!(cpsr&CPSR_V))&&(cpsr&CPSR_N)))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xC: //b gt Z==0 and N == V
                	trace_instr16("bgt 0x%08X\n", new_pc - 3);

                	if (cpsr&CPSR_Z)
                		return true;

                	if (   ((cpsr&CPSR_N) &&  (cpsr&CPSR_V))
                		|| ((!(cpsr&CPSR_N))&&(!(cpsr&CPSR_V))))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xD: //b le Z==1 or N != V
                	trace_instr16("ble 0x%08X\n", new_pc - 3);

                	if (   ((cpsr&CPSR_N) &&  (cpsr&CPSR_V))
                		|| ((!(cpsr&CPSR_N))&&(!(cpsr&CPSR_V)))
                		|| (cpsr&CPSR_Z))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0xE:
                	printf("Undefined instruction!");
                	return false;

                case 0xF:
                	printf("SWI");
                	return false;
			}

			return true;
		}
	},

	//B(2) unconditional branch
	{
		.mask = 0xF800,
		.instr = 0xE000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t new_pc = (instr >> 0) & 0x7FF;

			if (new_pc & (1 << 10))
				new_pc |= (~0) << 11;

			new_pc = (new_pc << 1) + mcu_read_reg(mcu, REG_PC) + 2;

			trace_instr16("B 0x%08X\n", new_pc - 3);

			mcu_write_reg(mcu, REG_PC, new_pc);

			return true;
		}
	},

	//BIC
	{
		.mask = 0xFFC0,
		.instr = 0x4380,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

			trace_instr16("bics r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c =  a & ~b;

			mcu_write_reg(mcu, reg, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//BKPT
	{
		.mask = 0xFF00,
		.instr = 0xBE00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t a  = (instr >> 0) & 0xFF;

			mcu_halt(mcu, HAL_TRAP);

			return true;
		}
	},

	//BL/BLX(1) H=b10
	{
		.mask = 0xF800,
		.instr = 0xF000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t a =instr & ((1 << 11) - 1);

			//trace_instr16("??\n", "");

            if(a & (1<<10))
            	a |= ~((1 << 11) - 1); //sign extend

            a = (a << 12) + mcu_read_reg(mcu, REG_PC);

            trace_instr16("bl 0x%08X\n", a - 3);

            mcu_write_reg(mcu, REG_LR, a);

			return true;
		}
	},

	//BL/BLX(1) H=b11, branch to thumb
	{
		.mask = 0xF800,
		.instr = 0xF800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t a = (instr & ((1 << 11) - 1)) << 1;

            a = a + mcu_read_reg(mcu, REG_LR) + 2;

            trace_instr16("bl 0x%08X\n", a - 3);

            mcu_write_reg(mcu, REG_LR, (mcu_read_reg(mcu, REG_PC) - 2) | 1);
            mcu_write_reg(mcu, REG_PC, a);

			return true;
		}
	},

	//BL/BLX(1) H=b01, branch to arm
	{
		.mask = 0xF800,
		.instr = 0xE800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			printf("Cannot branch to arm!");

			return false;
		}
	},

	//BLX(2)
	{
		.mask = 0xFF87,
		.instr = 0x4780,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src = (instr >> 3) & 0xF;

			trace_instr16("blx r%u\n", src);

			uint32_t new_pc = mcu_read_reg(mcu, src) + 2;

			if (new_pc & 1) {
            	mcu_write_reg(mcu, REG_LR, (mcu_read_reg(mcu, REG_PC) - 2) | 1);
            	new_pc &= ~1;
            	mcu_write_reg(mcu, REG_PC, new_pc);
        	}
        	else {
        		printf("Cannot branch to arm!");
        		return false;
        	}

			return true;
		}
	},

	//BX
	{
		.mask = 0xFF87,
		.instr = 0x4700,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src = (instr >> 3) & 0xF;

			trace_instr16("bx r%u\n", src);

			uint32_t new_pc = mcu_read_reg(mcu, src) + 2;

			if (new_pc & 1) {
            	new_pc &= ~1;
            	mcu_write_reg(mcu, REG_PC, new_pc);
        	}
        	else {
        		printf("Cannot branch to arm!");
        		return false;
        	}

			return true;
		}
	},

	//CMN
	{
		.mask = 0xFFC0,
		.instr = 0x42C0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src1 = (instr >> 0) & 0xF;
			reg_t src2 = (instr >> 3) & 0xF;

			trace_instr16("cmns r%u,r%u\n", src1, src2);

			uint32_t a = mcu_read_reg(mcu, src1);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a + b;

			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_cflag(mcu, a, b, 0);
			mcu_update_vflag(mcu, a, b, 0);

			return true;
		}
	},

	//CMP(1) compare immediate
	{
		.mask = 0xF800,
		.instr = 0x2800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src    = (instr >> 8) & 0x7;
			uint32_t imm = (instr >> 0) & 0xFF;

			trace_instr16("cmp r%u,#0x%02X", src, imm);

			uint32_t a = mcu_read_reg(mcu, src);

			uint32_t c = a - imm;

			trace_print(" ; %d, %d\n", a, c);

			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_cflag(mcu, a, ~imm, 1);
			mcu_update_vflag(mcu, a, ~imm, 1);

			return true;
		}
	},

	//CMP(2) compare register
	{
		.mask = 0xFFC0,
		.instr = 0x4280,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src1 = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

			trace_instr16("cmps r%u,r%u\n", src1, src2);

			uint32_t a = mcu_read_reg(mcu, src1);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a - b;

			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_cflag(mcu, a, ~b, 1);
			mcu_update_vflag(mcu, a, ~b, 1);

			return true;
		}
	},

	//CMP(3) compare high register
	{
		.mask = 0xFF00,
		.instr = 0x4500,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			if (((instr >> 6) & 3) == 0x0)
        	{
            	printf("UNPREDICTABLE");
            	return false;
        	}

			reg_t src1 = ((instr >> 0) & 0x7) | ((instr >> 4) & 0x8);
			reg_t src2 = (instr >> 3) & 0xF;

			trace_instr16("cmps r%u,r%u\n", src1, src2);

			if (src1 == 0xF)
			{
            	printf("UNPREDICTABLE");
            	return false;
        	}

			uint32_t a = mcu_read_reg(mcu, src1);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a - b;

			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_cflag(mcu, a, ~b, 1);
			mcu_update_vflag(mcu, a, ~b, 1);

			return true;
		}
	},

	// TODO: CPS

	//CPY copy high register
	{
		.mask = 0xFFC0,
		.instr = 0x4600,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src  = (instr >> 3) & 0x7;
			reg_t dest = (instr >> 0) & 0x7;

			uint32_t val = mcu_read_reg(mcu, src);

			trace_instr16("cpy r%u,r%u\t\t; val = %x\n", dest, src, val);

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//EOR
	{
		.mask = 0xFFC0,
		.instr = 0x4040,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

			trace_instr16("eors r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a ^ b;

			mcu_write_reg(mcu, reg, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//LDMIA
	{
		.mask = 0xF800,
		.instr = 0xC800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 8) & 0x7;

			bool first = true;
			trace_instr16("ldmia r%u, {", reg);

			uint32_t sp = mcu_read_reg(mcu, reg);

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val;

					if (first)
						first = false;
					else
						trace_print(", ");
					trace_print("r%u", reg);

					if (!mcu_fetch32(mcu, sp, &val)) {
						mcu_fetch_error(mcu, sp);
						return false;
					}

					mcu_write_reg(mcu, reg, val);
					sp += 4;
				}
			}

			mcu_write_reg(mcu, reg, sp);

			trace_print("}\n");

			return true;
		}
	},

	//LDR(1) two register immediate
	{
		.mask = 0xF800,
		.instr = 0x6800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src    = (instr >> 3) & 0x7;
			reg_t dest   = (instr >> 0) & 0x7;
			uint32_t imm = ((instr >> 6) & 0x1F) << 2;

			trace_instr16("ldr r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint32_t val;

			if (!mcu_fetch32(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);

				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDR(2) three register
	{
		.mask = 0xFE00,
		.instr = 0x5800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;
			reg_t dest   = (instr >> 0) & 0x7;

			trace_instr16("ldr r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint32_t val;

			if (!mcu_fetch32(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDR(3) pc + imm
	{
		.mask = 0xF800,
		.instr = 0x4800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = ((instr >> 0) & 0xFF) << 2;
			reg_t dest   = (instr >> 8) & 0x7;

			trace_instr16("ldr r%u,[PC+#0x%X]\n", dest, imm);

			uint32_t addr = (mcu_read_reg(mcu, REG_PC) & ~3) + imm;
			uint32_t val;

			if (!mcu_fetch32(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDR(4) sp + imm
	{
		.mask = 0xF800,
		.instr = 0x9800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = ((instr >> 0) & 0xFF) << 2;
			reg_t dest   = (instr >> 8) & 0x7;

			trace_instr16("ldr r%u,[SP+#0x%X]\n", dest, imm);

			uint32_t addr = mcu_read_reg(mcu, REG_SP) + imm;
			uint32_t val;

			if (!mcu_fetch32(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDRB(1)
	{
		.mask = 0xF800,
		.instr = 0x7800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = (instr >> 6) & 0x1F;
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;

			trace_instr16("ldrb r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint16_t val;

			if (!mcu_fetch16(mcu, addr & ~1, &val)) {
				mcu_fetch_error(mcu, addr & ~1);
				return false;
			}

			if (addr & 1) {
				val >>= 8;
			}

			mcu_write_reg(mcu, dest, val & 0xFF);

			return true;
		}
	},

	//LDRB(2)
	{
		.mask = 0xFE00,
		.instr = 0x5C00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("ldrb r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint16_t val;

			if (!mcu_fetch16(mcu, addr & ~1, &val)) {
				mcu_fetch_error(mcu, addr & ~1);
				return false;
			}

			if (addr & 1) {
				val >>= 8;
			}

			mcu_write_reg(mcu, dest, val & 0xFF);

			return true;
		}
	},

	//LDRH(1)
	{
		.mask = 0xF800,
		.instr = 0x8800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = ((instr >> 6) & 0x1F) << 1;
			reg_t dest   =  (instr >> 0) & 0x7;
			reg_t src    =  (instr >> 3) & 0x7;

			trace_instr16("ldrh r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint16_t val;

			if (!mcu_fetch16(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDRH(2)
	{
		.mask = 0xFE00,
		.instr = 0x5A00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("ldrh r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint16_t val;

			if (!mcu_fetch16(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDRSB
	{
		.mask = 0xFE00,
		.instr = 0x5600,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("ldrsb r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint16_t val;

			if (!mcu_fetch16(mcu, addr & ~1, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			if (addr & 1) {
				val >>= 8;
			}

			val &= 0xFF;
        	if(val & 0x80)
        		val |= (~0) << 8;

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LDRSH
	{
		.mask = 0xFE00,
		.instr = 0x5E00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("ldrsh r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint16_t val;

			if (!mcu_fetch16(mcu, addr, &val)) {
				mcu_fetch_error(mcu, addr);
				return false;
			}

			val &= 0xFFFF;
        	if(val & 0x8000)
        		val |= (~0) << 16;

			mcu_write_reg(mcu, dest, val);

			return true;
		}
	},

	//LSL(1)
	{
		.mask = 0xF800,
		.instr = 0x0000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = (instr >> 6) & 0x1F;

			trace_instr16("lsls r%u,r%u,#0x%X\n", dest, src, imm);

			uint32_t a = mcu_read_reg(mcu, src);

			if (imm != 0) {
				mcu_update_cflag_bit(mcu, a & (1 << (32 - imm)));
            	a <<= imm;
			}

			mcu_write_reg(mcu, dest, a);
			mcu_update_nflag(mcu, a);
			mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//LSL(2) two register
	{
		.mask = 0xFFC0,
		.instr = 0x4080,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg   = (instr >> 0) & 0x7;
			reg_t src   = (instr >> 3) & 0x7;

			trace_instr16("lsls r%u,r%u\n", reg, src);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t shift = mcu_read_reg(mcu, src) & 0xFF;

			if(shift < 32)
	        {
	            mcu_update_cflag_bit(mcu, a & (1 << (32 - shift)));
	            a <<= shift;
	        }
	        else if(shift == 32)
	        {
	            mcu_update_cflag_bit(mcu, a & 1);
	            a = 0;
	        }
	        else if (shift != 0)
	        {
	            mcu_update_cflag_bit(mcu, 0);
	            a = 0;
	        }

			mcu_write_reg(mcu, reg, a);
			mcu_update_nflag(mcu, a);
			mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//LSR(1) two register immediate
	{
		.mask = 0xF800,
		.instr = 0x0800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = (instr >> 6) & 0x1F;

			trace_instr16("lsrs r%u,r%u,#0x%X\n", dest, src, imm);

			uint32_t a = mcu_read_reg(mcu, src);

			if (imm == 0) {
				mcu_update_cflag_bit(mcu, a & 0x80000000);
				a = 0;
			}
			else {
				mcu_update_cflag_bit(mcu, a & (1 << (imm - 1)));
            	a >>= imm;
			}

			mcu_write_reg(mcu, dest, a);
			mcu_update_nflag(mcu, a);
			mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//LSR(2) two register
	{
		.mask = 0xFFC0,
		.instr = 0x40C0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg   = (instr >> 0) & 0x7;
			reg_t src   = (instr >> 3) & 0x7;

			trace_instr16("lsrs r%u,r%u\n", reg, src);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t shift = mcu_read_reg(mcu, src) & 0xFF;

			if(shift < 32)
	        {
	            mcu_update_cflag_bit(mcu, a & (1 << (shift - 1)));
	            a >>= shift;
	        }
	        else if(shift == 32)
	        {
	            mcu_update_cflag_bit(mcu, a & 0x80000000);
	            a = 0;
	        }
	        else if (shift != 0)
	        {
	            mcu_update_cflag_bit(mcu, 0);
	            a = 0;
	        }

			mcu_write_reg(mcu, reg, a);
			mcu_update_nflag(mcu, a);
			mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//MOV(1) immediate
	{
		.mask = 0xF800,
		.instr = 0x2000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 8) & 0x7;
			uint32_t imm = (instr >> 0) & 0xFF;

			trace_instr16("movs r%u,#0x%02X\n", dest, imm);

			mcu_write_reg(mcu, dest, imm);
			mcu_update_nflag(mcu, imm);
			mcu_update_zflag(mcu, imm);

			return true;
		}
	},

	//MOV(2) two low registers
	{
		.mask = 0xFFC0,
		.instr = 0x1C00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;

			trace_instr16("movs r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			mcu_write_reg(mcu, dest, a);
			mcu_update_nflag(mcu, a);
			mcu_update_zflag(mcu, a);
			mcu_update_cflag_bit(mcu, 0);
			mcu_update_vflag_bit(mcu, 0);

			return true;
		}
	},

	//MOV(3)
	{
		.mask = 0xFF00,
		.instr = 0x4600,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = ((instr >> 0) & 0x7) | ((instr >> 4) & 0x8);
			reg_t src    = (instr >> 3) & 0xF;

			trace_instr16("mov r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			if (dest == REG_PC) {
				a = (a & ~1) + 2;
			}

			mcu_write_reg(mcu, dest, a);

			return true;
		}
	},

	//MUL
	{
		.mask = 0xFFC0,
		.instr = 0x4340,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg    = (instr >> 0) & 0x7;
			reg_t src2   = (instr >> 3) & 0x7;

			trace_instr16("muls r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a * b;

			mcu_write_reg(mcu, reg, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//MVN
	{
		.mask = 0xFFC0,
		.instr = 0x43C0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;

			trace_instr16("mvns r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			uint32_t c = ~a;

			mcu_write_reg(mcu, dest, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//NEG
	{
		.mask = 0xFFC0,
		.instr = 0x4240,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;

			trace_instr16("negs r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			uint32_t c = 0 - a;

			mcu_write_reg(mcu, dest, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_cflag(mcu, 0, ~a, 1);
			mcu_update_vflag(mcu, 0, ~a, 1);

			return true;
		}
	},

	//ORR
	{
		.mask = 0xFFC0,
		.instr = 0x4300,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg    = (instr >> 0) & 0x7;
			reg_t src2   = (instr >> 3) & 0x7;

			trace_instr16("orrs r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a | b;

			mcu_write_reg(mcu, reg, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);

			return true;
		}
	},

	//POP
	{
		.mask = 0xFE00,
		.instr = 0xBC00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t sp = mcu_read_reg(mcu, REG_SP);

			bool first = true;
			trace_instr16("pop {");

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val;

					if (!first)
						trace_print(", ");
					else
						first = false;
					trace_print("r%u", reg);

					if (!mcu_fetch32(mcu, sp, &val)) {
						printf("Fetch faild!");
						return false;
					}

					mcu_write_reg(mcu, reg, val);
					sp += 4;
				}
			}

			if (instr & 0x100) {
				uint32_t val;

				if (!first)
					trace_print(", ");
				else
					first = false;

				trace_print("pc");

				if (!mcu_fetch32(mcu, sp, &val)) {
					printf("Fetch faild!");
					return false;
				}

				if ((val & 1) == 0) {
					printf("Pop with arm address");
					return false;
				}

				val += 2;
				mcu_write_reg(mcu, REG_PC, val);
				sp += 4;
			}

			mcu_write_reg(mcu, REG_SP, sp);

			trace_print("}\n");

			return true;
		}
	},

	//PUSH
	{
		.mask = 0xFE00,
		.instr = 0xB400,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t sp = mcu_read_reg(mcu, REG_SP);

			bool first = true;
			trace_instr16("push {");

			uint8_t num = 0;

			for (reg_t reg = 0; reg < 8; ++reg)
				if (instr & (1 << reg))
					num++;

			if (instr & 0x100)
				num++;

			sp -= num << 2;

			mcu_write_reg(mcu, REG_SP, sp);

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val = mcu_read_reg(mcu, reg);

					if (!first)
						trace_print(", ");
					else
						first = false;
					trace_print("r%u", reg);

					if (!mcu_write32(mcu, sp, val)) {
						mcu_write_error(mcu, sp);

						return false;
					}

					sp += 4;
				}
			}

			if (instr & 0x100) {
				uint32_t val = mcu_read_reg(mcu, REG_LR);

				if (!first)
					trace_print(", ");
				else
					first = false;

				trace_print("lr");

				if (!mcu_write32(mcu, sp, val)) {
					printf("Fetch faild!");
					return false;
				}
				sp += 4;
			}

			trace_print("}\n");

			return true;
		}
	},

	//REV
	{
		.mask = 0xFFC0,
		.instr = 0xBA00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr >> 3) & 0x7;

			trace_instr16("rev r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);
			uint32_t c;

			c  = ((a >>  0) & 0xFF) << 24;
        	c |= ((a >>  8) & 0xFF) << 16;
        	c |= ((a >> 16) & 0xFF) <<  8;
        	c |= ((a >> 24) & 0xFF) <<  0;

        	mcu_write_reg(mcu, dest, c);

			return true;
		}
	},

	//REV16
	{
		.mask = 0xFFC0,
		.instr = 0xBA40,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr >> 3) & 0x7;

			trace_instr16("rev16 r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);
			uint32_t c;

			c  = ((a >>  0) & 0xFF) << 24;
        	c |= ((a >>  8) & 0xFF) << 16;
        	c |= ((a >> 16) & 0xFF) <<  8;
        	c |= ((a >> 24) & 0xFF) <<  0;

        	mcu_write_reg(mcu, dest, c);

			return true;
		}
	},

	//REVSH
	{
		.mask = 0xFFC0,
		.instr = 0xBAC0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr >> 3) & 0x7;

			trace_instr16("revsh r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);
			uint32_t c;

			c  = ((a >> 0) & 0xFF) << 8;
        	c |= ((a >> 8) & 0xFF) << 0;

        	if(c & 0x8000)
        		c |= 0xFFFF0000;
        	else
        	    c &= 0x0000FFFF;

        	mcu_write_reg(mcu, dest, c);

			return true;
		}
	},

	//ROR
	{
		.mask = 0xFFC0,
		.instr = 0xBAC0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

			trace_instr16("rors r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			if (b != 0) {
				b &= 0x1F;

				if (b == 0) {
					mcu_update_cflag_bit(mcu, a & 0x80000000);
				}
				else {
					mcu_update_cflag_bit(mcu, a & (1 << (b - 1)));
                	uint32_t temp = a << (32 - b);
                	a >>= b;
                	a |= temp;
				}
			}

        	mcu_write_reg(mcu, reg, a);
        	mcu_update_nflag(mcu, a);
        	mcu_update_zflag(mcu, a);

			return true;
		}
	},

	//SBC
	{
		.mask = 0xFFC0,
		.instr = 0x4180,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 0) & 0x7;
			reg_t src2 = (instr >> 3) & 0x7;

			trace_instr16("sbc r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a - b;

			if (!(mcu_read_reg(mcu, REG_APSR) & CPSR_C))
				c--;

        	mcu_write_reg(mcu, reg, c);
        	mcu_update_nflag(mcu, c);
        	mcu_update_zflag(mcu, c);

        	if(mcu_read_reg(mcu, REG_APSR) & CPSR_C) {
	            mcu_update_cflag(mcu, a, ~b, 1);
	            mcu_update_vflag(mcu, a, ~b, 1);
        	}
	        else {
            	mcu_update_cflag(mcu, a, ~b, 0);
	            mcu_update_vflag(mcu, a, ~b, 0);
        	}

			return true;
		}
	},

	// TODO: SETEND

	//STMIA
	{
		.mask = 0xF800,
		.instr = 0xC000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg  = (instr >> 8) & 0x7;

			bool first = true;
			trace_instr16("stmia r%u, {\n", reg);

			uint32_t sp = mcu_read_reg(mcu, reg);

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val = mcu_read_reg(mcu, reg);

					if (first)
						first = false;
					else
						trace_print(", ");
					trace_print("r%u", reg);

					if (!mcu_write32(mcu, sp, val)) {
						mcu_write_error(mcu, sp);
						return false;
					}

					sp += 4;
				}
			}

			mcu_write_reg(mcu, reg, sp);
			trace_print("}\n");

			return true;
		}
	},

	//STR(1)
	{
		.mask = 0xF800,
		.instr = 0x6000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = ((instr >> 6) & 0x1F) << 2;

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint32_t val = mcu_read_reg(mcu, dest);

			trace_instr16("str r%u,[r%u,#0x%X]\t; r%u = %x, addr = %x\n", dest, src, imm, dest, val, addr);

			if (!mcu_write32(mcu, addr, val)) {
				mcu_write_error(mcu, addr);
				return false;
			}

			return true;
		}
	},

	//STR(2)
	{
		.mask = 0xFE00,
		.instr = 0x5000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint32_t val = mcu_read_reg(mcu, dest);

			trace_instr16("str r%u,[r%u,r%u]\t; r%u = %x, addr = %x\n", dest, src1, src2, dest, val, addr);

			if (!mcu_write32(mcu, addr, val)) {
				mcu_write_error(mcu, val);
				return false;
			}

			return true;
		}
	},

	//STR(3)
	{
		.mask = 0xF800,
		.instr = 0x9000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 8) & 0x7;
			uint32_t imm = ((instr >> 0) & 0xFF) << 2;

			uint32_t addr = mcu_read_reg(mcu, REG_SP) + imm;
			uint32_t val = mcu_read_reg(mcu, dest);

			trace_instr16("str r%u,[SP,#0x%X]\t; r%u = %x, addr = %x\n", dest, imm, dest, val, addr);

			if (!mcu_write32(mcu, addr, val)) {
				mcu_write_error(mcu, addr);
				return false;
			}

			return true;
		}
	},

	//STRB(1)
	{
		.mask = 0xF800,
		.instr = 0x7000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = (instr >> 6) & 0x1F;

			trace_instr16("strb r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint16_t val;

			if (!mcu_fetch16(mcu, addr & (~1), &val)) {
				mcu_fetch_error(mcu, addr & ~1);
				return false;
			}

			if (addr & 1) {
				val &= 0x00FF;
			 	val |= mcu_read_reg(mcu, dest) << 8;
			}
			else {
				val &= 0xFF00;
				val |= mcu_read_reg(mcu, dest) & 0xFF;
			}

			if (!mcu_write32(mcu, addr & (~1), val)) {
				mcu_write_error(mcu, val);
				return false;
			}

			return true;
		}
	},

	//STRB(2)
	{
		.mask = 0xFE00,
		.instr = 0x5400,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("strb r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint16_t val;

			if (!mcu_fetch16(mcu, addr & (~1), &val)) {
				mcu_fetch_error(mcu, addr & ~1);
				return false;
			}

			if (addr & 1) {
				val &= 0x00FF;
			 	val |= mcu_read_reg(mcu, dest) << 8;
			}
			else {
				val &= 0xFF00;
				val |= mcu_read_reg(mcu, dest) & 0xFF;
			}

			if (!mcu_write32(mcu, addr & (~1), val)) {
				mcu_write_error(mcu, val);
				return false;
			}

			return true;
		}
	},

	//STRH(1)
	{
		.mask = 0xF800,
		.instr = 0x8000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = ((instr >> 6) & 0x1F) << 1;

			trace_instr16("strh r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint32_t val = mcu_read_reg(mcu, dest);

			if (!mcu_write16(mcu, addr, val)) {
				mcu_write_error(mcu, val);
				return false;
			}

			return true;
		}
	},

	//STRH(2)
	{
		.mask = 0xFE00,
		.instr = 0x5200,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("strh r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint32_t val = mcu_read_reg(mcu, dest);

			if (!mcu_write16(mcu, addr, val)) {
				mcu_write_error(mcu, val);
				return false;
			}

			return true;
		}
	},

	//SUB(1)
	{
		.mask = 0xFE00,
		.instr = 0x1E00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src    = (instr >> 3) & 0x7;
			uint32_t imm = (instr >> 6) & 0x7;

			trace_instr16("subs r%u,r%u,#0x%X\n", dest, src, imm);

			uint32_t a = mcu_read_reg(mcu, src);
			uint32_t c = a - imm;

			mcu_write_reg(mcu, dest, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_vflag(mcu, a, ~imm, 1);
			mcu_update_cflag(mcu, a, ~imm, 1);

			return true;
		}
	},

	//SUB(2)
	{
		.mask = 0xF800,
		.instr = 0x3800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t reg   = (instr >> 8) & 0x7;
			uint32_t imm = (instr >> 0) & 0xFF;

			trace_instr16("subs r%u,#0x%02X\n", reg, imm);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t c = a - imm;

			mcu_write_reg(mcu, reg, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_vflag(mcu, a, ~imm, 1);
			mcu_update_cflag(mcu, a, ~imm, 1);

			return true;
		}
	},

	//SUB(3)
	{
		.mask = 0xFE00,
		.instr = 0x1A00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest   = (instr >> 0) & 0x7;
			reg_t src1   = (instr >> 3) & 0x7;
			reg_t src2   = (instr >> 6) & 0x7;

			trace_instr16("subs r%u,r%u,r%u\n", dest, src1, src2);

			uint32_t a = mcu_read_reg(mcu, src1);
			uint32_t b = mcu_read_reg(mcu, src2);
			uint32_t c = a - b;

			mcu_write_reg(mcu, dest, c);
			mcu_update_nflag(mcu, c);
			mcu_update_zflag(mcu, c);
			mcu_update_vflag(mcu, a, ~b, 1);
			mcu_update_cflag(mcu, a, ~b, 1);

			return true;
		}
	},

	//SUB(4)
	{
		.mask = 0xFF80,
		.instr = 0xB080,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = ((instr >> 0) & 0x7F) << 2;

			trace_instr16("sub SP,#0x%02X\n", imm);

			mcu_write_reg(mcu, REG_SP, mcu_read_reg(mcu, REG_SP) - imm);

			return true;
		}
	},

	//SUB(4)
	{
		.mask = 0xFF00,
		.instr = 0xDF00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t imm = (instr >> 0) & 0xFF;

			trace_instr16("swi 0x%02X\n", imm);

			printf("unkown swi 0x%02x\n", imm);

			return false;
		}
	},

	//SXTB
	{
		.mask = 0xFFC0,
		.instr = 0xB240,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr > 3) & 0x7;

			trace_instr16("sxtb r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			a &= 0xFF;

			if (a & 0x80)
				a |= (~0) << 8;

			mcu_write_reg(mcu, dest, a);

			return true;
		}
	},

	//SXTH
	{
		.mask = 0xFFC0,
		.instr = 0xB200,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr > 3) & 0x7;

			trace_instr16("sxth r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			a &= 0xFFFF;

			if (a & 0x8000)
				a |= (~0) << 16;

			mcu_write_reg(mcu, dest, a);

			return true;
		}
	},

	//TST
	{
		.mask = 0xFFC0,
		.instr = 0x4200,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t src1 = (instr >> 0) & 0x7;
			reg_t src2  = (instr > 3) & 0x7;

			trace_instr16("tst r%u,r%u\n", src1, src2);

			uint32_t a = mcu_read_reg(mcu, src1);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a & b;

			mcu_update_zflag(mcu, c);
			mcu_update_nflag(mcu, c);

			return true;
		}
	},

	//UXTB
	{
		.mask = 0xFFC0,
		.instr = 0xB2C0,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr > 3) & 0x7;

			trace_instr16("uxtb r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			a &= 0xFF;

			mcu_write_reg(mcu, dest, a);

			return true;
		}
	},

	//UXTH
	{
		.mask = 0xFFC0,
		.instr = 0xB280,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			reg_t dest = (instr >> 0) & 0x7;
			reg_t src  = (instr > 3) & 0x7;

			trace_instr16("uxth r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			a &= 0xFFFF;

			mcu_write_reg(mcu, dest, a);

			return true;
		}
	},

	// WFE
	{
		.mask = 0xFFFF,
		.instr = 0xBF20,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			// TODO do actually wait here

			return true;
		}
	},

	// WFI
	{
		.mask = 0xFFFF,
		.instr = 0xBF30,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			// TODO do actually wait here

			return true;
		}
	},

	{ 0, 0, NULL }
};

struct mcu_instr32 mcu_instr32_cortex_m0p[] = {

	// MSR
	{
		.mask = 0xFFE00000,
		.instr = 0xF3800000,
		.impl = ^bool(mcu_t mcu, uint32_t instr) {
			reg_t   src  = (instr >> 16) & 0x7;
			uint8_t sysm = (instr >>  0) & 0x7F;

			switch (sysm) {
				case 0x0:
					trace_instr32("msr APSR, r%u\n", src);
					mcu_write_reg(mcu, REG_APSR, mcu_read_reg(mcu, src));
				case 0x8:
					trace_instr32("msr MSP, r%u\n", src);
					mcu_write_reg(mcu, REG_MSP, mcu_read_reg(mcu, src));
					break;
				case 0x9:
					trace_instr32("msr PSP, r%u\n", src);
					mcu_write_reg(mcu, REG_PSP, mcu_read_reg(mcu, src));
					break;
				// case 0x10:
				// 	trace_instr32("msr PRIMASK, r%u\n", src);
				// 	mcu_write_reg(mcu, REG_PRIMASK, mcu_read_reg(mcu, src));
				// 	break;
				case 0x14:
					trace_instr32("msr CONTROL, r%u\n", src);
					mcu_write_reg(mcu, REG_CONTROL, mcu_read_reg(mcu, src));
					break;
				default:
					trace_instr32("msr <unkown>, r%u\n", src);
					mcu_halt(mcu, HALT_UNKOWN_INSTRUCTION);
					return false;
			}

			return true;
		}
	},

	// MRS
	{
		.mask = 0xFFE00000,
		.instr = 0xF3E00000,
		.impl = ^bool(mcu_t mcu, uint32_t instr) {
			reg_t   dest = (instr >> 8) & 0xF;
			uint8_t sysm = (instr >> 0) & 0x7F;

			switch (sysm) {
				case 0x0:
				case 0x1:
				case 0x2:
				case 0x3:
				case 0x4:
				case 0x5:
				case 0x6:
				case 0x7:
				{
					trace_instr32("mrs r%u, {", dest);
					bool first = true;

					uint32_t val = 0;

					if (sysm & (1 << 0)) {
						first = false;
						trace_print("IPSR");

						val |= mcu_read_reg(mcu, REG_IPSR);
					}

					if (sysm & (1 << 1)) {
						if (first)
							first = false;
						else
							trace_print(",");
						trace_print("EPSR");

						// From ARMv6 ARM, B4-309:
						//
						// None of the EPSR bits are readable during normal execution. They
						// all Read-As-Zero when read using MRS. Halting debug can read the
						// EPSR bits using the register transfer mechanism.
					}

					if (sysm & (1 << 2)) {
						if (first)
							first = false;
						else
							trace_print(",");
						trace_print("APSR");

						val |= mcu_read_reg(mcu, REG_APSR);
					}

					mcu_write32(mcu, dest, val);
				}
				case 0x8:
					trace_instr32("mrs r%u, MSP\n", dest);
					mcu_write_reg(mcu, dest, mcu_read_reg(mcu, REG_MSP));
					break;
				case 0x9:
					trace_instr32("mrs r%u, PSP\n", dest);
					mcu_write_reg(mcu, dest, mcu_read_reg(mcu, REG_PSP));
					break;
				// case 0x10:
				// 	trace_instr32("mrs r%u, PRIMASK\n", dest);
				// 	mcu_write_reg(mcu, dest, mcu_read_reg(mcu, REG_PRIMASK));
				// 	break;
				case 0x14:
					trace_instr32("mrs r%u, CONTROL\n", dest);
					mcu_write_reg(mcu, dest, mcu_read_reg(mcu, REG_CONTROL));
					break;
				default:
					trace_instr32("mrs r%u, <unkown>\n", dest);
					mcu_halt(mcu, HALT_UNKOWN_INSTRUCTION);
					return false;
			}

			return true;
		}
	},

	//BL
	{
		.mask = 0xF8000000,
		.instr = 0xF0000000,
		.impl = ^bool(mcu_t mcu, uint32_t instr) {
			uint32_t imm;
			uint32_t addr;
			uint8_t sign = (instr >> 26) & 0x1;
			uint8_t j1   = (instr >> 13) & 0x1;
			uint8_t j2   = (instr >> 11) & 0x1;

			imm = (!(j1 ^ sign) << 23) | (!(j2 ^ sign) << 22) | (((instr >> 16) & 0x3FF) << 12) | ((instr & 0x7FF) << 1);

            if(sign)
            	imm |= 0xFF000000; //sign extend

            addr = imm + mcu_read_reg(mcu, REG_PC);

            trace_instr32("bl 0x%08X ; @0x%08x\n", imm, addr - 3);

            mcu_write_reg(mcu, REG_LR, (mcu_read_reg(mcu, REG_PC) - 2) | 1);
            mcu_write_reg(mcu, REG_PC, addr);

			return true;
		}
	},
};
