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

typedef struct mcu_cortex_m0p* mcu_cortex_m0p_t;

extern struct mcu_instr mcu_instr_cortex_m0p[];

enum {
	CPSR_N = (1<<31),
	CPSR_Z = (1<<30),
	CPSR_C = (1<<29),
	CPSR_V = (1<<28),
	CPSR_Q = (1<<27)
};

mcu_t mcu_cortex_m0p_create(size_t ramsize)
{
	mcu_cortex_m0p_t mcu = calloc(1, sizeof(struct mcu_cortex_m0p));

	if (!mcu) {
		perror("Could not allocate mcu struct");
		return NULL;
	}

	mcu->mcu.instrs = mcu_instr_cortex_m0p;

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

	return (mcu_t)mcu;
}

bool mcu_reset(mcu_t mcu)
{
	{
		uint32_t val;

		if (!mcu_fetch32(mcu, 0x0, &val)) {
			printf("Fetch faild");
			return false;
		}

		mcu_write_reg(mcu, REG_SP, val);
	}

	{
		uint32_t val;

		if (!mcu_fetch32(mcu, 0x4, &val)) {
			printf("Fetch faild");
			return false;
		}

		mcu_write_reg(mcu, REG_PC, val + 2);
	}

	return true;
}

uint32_t mcu_read_reg(mcu_t _mcu, reg_t reg)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	return mcu->regs[reg];
}

void mcu_write_reg(mcu_t _mcu, reg_t reg, uint32_t val)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	if (reg == REG_PC)
		val &= ~1;

	mcu->regs[reg] = val;
}

bool mcu_instr_step(mcu_t mcu)
{
	uint32_t pc = mcu_read_reg(mcu, REG_PC);
	uint16_t instr;

	if (!mcu_fetch16(mcu, pc - 2, &instr)) {
		printf("ERROR: could not fetch instruction. [pc=0x%x]", pc);
		return false;
	}

	mcu_write_reg(mcu, REG_PC, pc+2);

	for (mcu_instr_t def = mcu->instrs; def->impl != NULL; ++def) {
		if ((instr & def->mask) == def->instr) {
			return def->impl(mcu, instr);
		}
	}

	return false;
}

static void mcu_update_nflag(void* _mcu, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_CPSR);

	if( c & (1 << 31))
		cpsr |= CPSR_N;
	else
		cpsr &= ~CPSR_N;

	mcu_write_reg(_mcu, REG_CPSR, cpsr);
}

static void mcu_update_zflag(void* _mcu, uint32_t c)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_CPSR);

	if (c == 0) 
		cpsr |= CPSR_Z; 
	else 
		cpsr &= ~CPSR_Z;

	mcu_write_reg(_mcu, REG_CPSR, cpsr);
}

static void mcu_update_cflag_bit(void* _mcu, uint32_t val)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;
	uint32_t cpsr = mcu_read_reg(_mcu, REG_CPSR);

	if (val) 
    	cpsr |= CPSR_C;
    else 
    	cpsr &= ~CPSR_C;

    mcu_write_reg(_mcu, REG_CPSR, cpsr);
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
	uint32_t cpsr = mcu_read_reg(_mcu, REG_CPSR);

	if (val) 
    	cpsr |= CPSR_V;
    else 
    	cpsr &= ~CPSR_V;

    mcu_write_reg(_mcu, REG_CPSR, cpsr);
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

	mcu_halt((mcu_t)mcu, HALT_HARD_FAULT);
}

static void mcu_write_error(void* _mcu, uint32_t addr)
{
	mcu_cortex_m0p_t mcu = (mcu_cortex_m0p_t)_mcu;

	mcu_halt((mcu_t)mcu, HALT_HARD_FAULT);
}

struct mcu_instr mcu_instr_cortex_m0p[] = {
	// TODO: ADC

	//ADD(1) small immediate two registers
	{
		.mask = 0xFE00,
		.instr = 0x1C00,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint8_t dest = (instr >> 0) & 0x7;
        	uint8_t src  = (instr >> 3) & 0x7;
        	uint8_t b  = (instr >> 6) & 0x7;

        	trace_instr("adds r%u,r%u,#0x%X\n", dest, src, b);

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

        	trace_instr("adds r%u,#0x%02X\n", reg, b);

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

        	trace_instr("adds r%u,r%u,r%u\n", dest, src, src2);

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

        	trace_instr("add r%u,r%u\n", reg, src2);

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

			trace_instr("add r%u,PC,#0x%02X\n", dest, imm);

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

			trace_instr("add r%u,SP,#0x%02X\n", dest, imm);

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

			trace_instr("add SP,#0x%02X\n", imm);

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

			trace_instr("ands r%u,r%u\n", reg, src2);

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

			trace_instr("asrs r%u,r%u,#0x%X\n", dest, src, shift);

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

			trace_instr("asrs r%u,r%u\n", reg, shift_reg);

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

			uint32_t cpsr = mcu_read_reg(mcu, REG_CPSR);
			switch (op) {
				case 0x0: //b eq  z set
					trace_instr("beq 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_Z)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x1: //b ne  z clear
                	trace_instr("bne 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_Z))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x2: //b cs c set
                	trace_instr("bcs 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_C)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x3: //b cc c clear
                	trace_instr("bcc 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_C))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x4: //b mi n set
                	trace_instr("bmi 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_N)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x5: //b pl n clear
                	trace_instr("bpl 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_N))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x6: //b vs v set
                	trace_instr("bvs 0x%08X\n", new_pc - 3);

                	if(cpsr & CPSR_V)
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x7: //b vc v clear
                	trace_instr("bvc 0x%08X\n", new_pc - 3);

                	if(!(cpsr & CPSR_V))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0x8: //b hi c set z clear
                	trace_instr("bhi 0x%08X\n", new_pc - 3);

                	if((cpsr & CPSR_C) && !(cpsr & CPSR_Z))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0x9: //b ls c clear or z set
                	trace_instr("bls 0x%08X\n", new_pc - 3);

                	if((cpsr & CPSR_Z) || !(cpsr & CPSR_C))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;

                case 0xA: //b ge N == V
                	trace_instr("bge 0x%08X\n", new_pc - 3);

                	if (     ((cpsr & CPSR_N)  &&  (cpsr & CPSR_V))
                		|| ((!(cpsr & CPSR_N)) && !(cpsr & CPSR_V)))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xB: //b lt N != V
                	trace_instr("blt 0x%08X\n", new_pc - 3);

                	if (   ((!(cpsr&CPSR_N))&&(cpsr&CPSR_V))
                		|| ((!(cpsr&CPSR_V))&&(cpsr&CPSR_N)))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xC: //b gt Z==0 and N == V
                	trace_instr("bgt 0x%08X\n", new_pc - 3);

                	if (cpsr&CPSR_Z)
                		return true;

                	if (   ((cpsr&CPSR_N) &&  (cpsr&CPSR_V))
                		|| ((!(cpsr&CPSR_N))&&(!(cpsr&CPSR_V))))
                    	mcu_write_reg(mcu, REG_PC, new_pc);
                	return true;
                case 0xD: //b le Z==1 or N != V
                	trace_instr("ble 0x%08X\n", new_pc - 3);

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

			trace_instr("B 0x%08X\n", new_pc - 3);

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

			trace_instr("bics r%u,r%u\n", reg, src2);

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
			
			printf("Breakpoint %d\n", a);
			
			return false;
		}
	},

	//BL/BLX(1) H=b10
	{
		.mask = 0xF800,
		.instr = 0xF000,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t a =instr & ((1 << 11) - 1);

			//trace_instr("??\n", "");

            if(a & (1<<10)) 
            	a |= ~((1 << 11) - 1); //sign extend

            a = (a << 12) + mcu_read_reg(mcu, REG_PC);

            trace_instr("bl 0x%08X\n", a - 3);

            mcu_write_reg(mcu, REG_LC, a);
			
			return true;
		}
	},

	//BL/BLX(1) H=b11, branch to thumb
	{
		.mask = 0xF800,
		.instr = 0xF800,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t a = (instr & ((1 << 11) - 1)) << 1;

            a = a + mcu_read_reg(mcu, REG_LC) + 2;

            trace_instr("bl 0x%08X\n", a - 3);

            mcu_write_reg(mcu, REG_LC, (mcu_read_reg(mcu, REG_PC) - 2) | 1);
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

			trace_instr("blx r%u\n", src);

			uint32_t new_pc = mcu_read_reg(mcu, src) + 2;

			if (new_pc & 1) {
            	mcu_write_reg(mcu, REG_LC, (mcu_read_reg(mcu, REG_PC) - 2) | 1);
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

			trace_instr("bx r%u\n", src);

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

			trace_instr("cmns r%u,r%u\n", src1, src2);

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
			reg_t src    = (instr >> 8) & 0xF;
			uint32_t imm = (instr >> 0) & 0xFF;

			trace_instr("cmp r%u,#0x%02X\n", src, imm);

			uint32_t a = mcu_read_reg(mcu, src);

			uint32_t c = a - imm;

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
			reg_t src1 = (instr >> 0) & 0xF;
			reg_t src2 = (instr >> 3) & 0xF;

			trace_instr("cmps r%u,r%u\n", src1, src2);

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

			trace_instr("cmps r%u,r%u\n", src1, src2);

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

			trace_instr("cpy r%u,r%u\n", dest, src);

			mcu_write_reg(mcu, dest, mcu_read_reg(mcu, src));
			
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

			trace_instr("eors r%u,r%u\n", reg, src2);

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

			trace_instr("LDMIA r%u\n", reg);

			uint32_t sp = mcu_read_reg(mcu, reg);

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val;

					if (!mcu_fetch32(mcu, sp, &val)) {
						mcu_fetch_error(mcu, sp);
						return false;
					}

					mcu_write_reg(mcu, reg, val);
					sp += 4;
				}
			}

			mcu_write_reg(mcu, reg, sp);
			
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

			trace_instr("ldr r%u,[r%u,#0x%X]\n", dest, src, imm);

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

			trace_instr("ldr r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("ldr r%u,[PC+#0x%X]\n", dest, imm);

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

			trace_instr("ldr r%u,[SP+#0x%X]\n", dest, imm);

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

			trace_instr("ldrb r%u,[r%u,#0x%X]\n", dest, src, imm);

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

			trace_instr("ldrb r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("ldrh r%u,[r%u,#0x%X]\n", dest, src, imm);

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

			trace_instr("ldrh r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("ldrsb r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("ldrsh r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("lsls r%u,r%u,#0x%X\n", dest, src, imm);

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

			trace_instr("lsls r%u,r%u\n", reg, src);

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

			trace_instr("lsrs r%u,r%u,#0x%X\n", dest, src, imm);

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

			trace_instr("lsrs r%u,r%u\n", reg, src);

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

			trace_instr("movs r%u,#0x%02X\n", dest, imm);

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

			trace_instr("movs r%u,r%u\n", dest, src);

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

			trace_instr("mov r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			if (dest == REG_PC) {
				a = a & ~1 + 2;
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

			trace_instr("muls r%u,r%u\n", reg, src2);

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

			trace_instr("mvns r%u,r%u\n", dest, src);

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

			trace_instr("negs r%u,r%u\n", dest, src);

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

			trace_instr("orrs r%u,r%u\n", reg, src2);

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

			printf("pop\n");

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val;

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
			
			return true;
		}
	},

	//PUSH
	{
		.mask = 0xFE00,
		.instr = 0xB400,
		.impl = ^bool(mcu_t mcu, uint16_t instr) {
			uint32_t sp = mcu_read_reg(mcu, REG_SP);

			printf("push\n");

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

					if (!mcu_write32(mcu, sp, val)) {
						mcu_write_error(mcu, sp);
						
						return false;
					}

					sp += 4;
				}
			}

			if (instr & 0x100) {
				uint32_t val = mcu_read_reg(mcu, REG_LC);

				if (!mcu_write32(mcu, sp, val)) {
					printf("Fetch faild!");
					return false;
				}
				sp += 4;
			}

			
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

			trace_instr("rev r%u,r%u\n", dest, src);

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

			trace_instr("rev16 r%u,r%u\n", dest, src);

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

			trace_instr("revsh r%u,r%u\n", dest, src);

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

			trace_instr("rors r%u,r%u\n", reg, src2);

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

			trace_instr("sbc r%u,r%u\n", reg, src2);

			uint32_t a = mcu_read_reg(mcu, reg);
			uint32_t b = mcu_read_reg(mcu, src2);

			uint32_t c = a - b;

			if (!(mcu_read_reg(mcu, REG_CPSR) & CPSR_C)) 
				c--;

        	mcu_write_reg(mcu, reg, c);
        	mcu_update_nflag(mcu, c);
        	mcu_update_zflag(mcu, c);

        	if(mcu_read_reg(mcu, REG_CPSR) & CPSR_C) {
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

			printf("stmia\n");

			uint32_t sp = mcu_read_reg(mcu, reg);

			for (reg_t reg = 0; reg < 8; ++reg) {
				if (instr & (1 << reg)) {
					uint32_t val = mcu_read_reg(mcu, reg);

					if (!mcu_write32(mcu, sp, val)) {
						mcu_write_error(mcu, sp);
						return false;
					}

					sp += 4;
				}
			}

			mcu_write_reg(mcu, reg, sp);
			
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

			trace_instr("str r%u,[r%u,#0x%X]\n", dest, src, imm);

			uint32_t addr = mcu_read_reg(mcu, src) + imm;
			uint32_t val = mcu_read_reg(mcu, dest);

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

			trace_instr("str r%u,[r%u,r%u]\n", dest, src1, src2);

			uint32_t addr = mcu_read_reg(mcu, src1) + mcu_read_reg(mcu, src2);
			uint32_t val = mcu_read_reg(mcu, dest);

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

			trace_instr("str r%u,[SP,#0x%X]\n", dest, imm);

			uint32_t addr = mcu_read_reg(mcu, REG_SP) + imm;
			uint32_t val = mcu_read_reg(mcu, dest);

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

			trace_instr("strb r%u,[r%u,#0x%X]\n", dest, src, imm);

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

			trace_instr("strb r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("strh r%u,[r%u,#0x%X]\n", dest, src, imm);

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

			trace_instr("strh r%u,[r%u,r%u]\n", dest, src1, src2);

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

			trace_instr("subs r%u,r%u,#0x%X\n", dest, src, imm);

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

			trace_instr("subs r%u,#0x%02X\n", reg, imm);

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

			trace_instr("subs r%u,r%u,r%u\n", dest, src1, src2);

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
			uint32_t imm = ((instr >> 0) & 0x1F) << 2;

			trace_instr("sub SP,#0x%02X\n", imm);

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

			trace_instr("swi 0x%02X\n", imm);

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

			trace_instr("sxtb r%u,r%u\n", dest, src);

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

			trace_instr("sxth r%u,r%u\n", dest, src);

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

			trace_instr("tst r%u,r%u\n", src1, src2);

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

			trace_instr("uxtb r%u,r%u\n", dest, src);

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

			trace_instr("uxth r%u,r%u\n", dest, src);

			uint32_t a = mcu_read_reg(mcu, src);

			a &= 0xFFFF;

			mcu_write_reg(mcu, dest, a);
			
			return true;
		}
	},

	{ 0, 0, NULL }
};
