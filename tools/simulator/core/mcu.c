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

DECLARE_MEM_OP(fetch16, uint16_t*);
DECLARE_MEM_OP(fetch32, uint32_t*);
DECLARE_MEM_OP(write16, uint16_t);
DECLARE_MEM_OP(write32, uint32_t);

bool mcu_emu_fetch16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t* valueOut)
{
	uint32_t value;

	if (!mem_dev->fetch32(mcu, mem_dev, addr & ~2, &value))
		return false;

	if (addr & 2)
		*valueOut = (value >> 0) & 0xFFFF;
	else
		*valueOut = (value >> 16) & 0xFFFF;

	return true;
}

bool mcu_emu_write16(mcu_t mcu, mem_dev_t mem_dev, uint32_t addr, uint16_t valueIn)
{
	uint32_t value;

	if (!mem_dev->fetch32(mcu, mem_dev, addr & ~2, &value))
		return false;

	if (addr & 2)
		value = (value & ~0xFFFF) | ((valueIn << 0) & 0xFFFF);
	else
		value = (value &  0xFFFF) | ((valueIn << 16) & ~0xFFFF);

	return mem_dev->write32(mcu, mem_dev, addr & ~2, value);
}

bool mcu_util_fetch8(mcu_t mcu, uint32_t addr, uint8_t* valueOut)
{
	uint16_t value;

	if (!mcu_fetch16(mcu, addr & ~1, &value))
		return false;

	if (addr & 1)
		*valueOut = (value >> 8) & 0xFF;
	else
		*valueOut = (value >> 0) & 0xFF;

	return true;
}

bool mcu_add_mem_dev(mcu_t mcu, uint32_t offset, mem_dev_t dev) 
{
	// Quick and dirty, unordered

	dev->next = mcu->mem_devs;
	dev->offset = offset;
	mcu->mem_devs = dev;

	return true;
}

bool mcu_is_unlocked(mcu_t mcu)
{
	return mcu->unlocked;
}

bool mcu_unlock(mcu_t mcu)
{
	mcu->unlocked = true;

	return true;
}

bool mcu_lock(mcu_t mcu)
{
	mcu->unlocked = false;
	
	return true;
}

bool mcu_is_halted(mcu_t mcu)
{
	return mcu->state == mcu_halted;
}

halt_reason_t mcu_halt_reason(mcu_t mcu)
{
	return mcu->halt_reason;
}

bool mcu_halt(mcu_t mcu, halt_reason_t reason)
{
	if (mcu_is_halted(mcu))
		return true;

	mcu->state = mcu_halted;
	mcu->halt_reason = reason;

	printf("=X CPU halted\n");

	for (mcu_callbacks_t callbacks = mcu->callbacks; callbacks != NULL; callbacks = callbacks->next)
		callbacks->mcu_did_halt(mcu, reason, callbacks->context);

	return true;
}

bool mcu_resume(mcu_t mcu)
{
	if (!mcu_is_halted(mcu))
		return true;

	mcu->state = mcu_running;

	printf("=> CPU resumed\n");

	return true;
}

bool mcu_runloop(mcu_t mcu)
{
	if (!mcu_is_halted(mcu)) {
		return mcu_instr_step(mcu);
	}

	return true;
}

bool mcu_step(mcu_t mcu)
{
	return mcu_instr_step(mcu);
}

void mcu_add_callbacks(mcu_t mcu, mcu_callbacks_t callbacks)
{
	callbacks->next = mcu->callbacks;
	mcu->callbacks = callbacks;
}

