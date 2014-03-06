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

#include <irq.h>
#include <runtime.h>

#include <stddef.h>

struct irq_entry {
	irq_handler_t handler;
};

typedef struct irq_entry irq_entry_t;

struct {
	irq_entry_t entries[NUMBER_OF_IRQS];
} irq;

void irq_init()
{
	for (uint8_t i = 0; i < NUMBER_OF_IRQS; i++)
		irq.entries[i].handler = NULL;
}

bool irq_register(uint8_t irq_number, irq_handler_t handler)
{
	if (irq.entries[irq_number].handler || irq_number >= NUMBER_OF_IRQS)
		return false;

	irq.entries[irq_number].handler = handler;

	return true;
}

bool irq_unregister(uint8_t irq_number, irq_handler_t handler)
{
	if (irq.entries[irq_number].handler != handler || irq_number >= NUMBER_OF_IRQS)
		return false;

	irq.entries[irq_number].handler = NULL;

	return true;
}

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

void do_irq(uint8_t irq_number)
{
	if (likely(irq.entries[irq_number].handler)) {
		irq.entries[irq_number].handler();
	}
	else {
		assert(irq_number != IRQ_HANDLER_MISSING, "IRQ missing handler is missing");

		return do_irq(IRQ_HANDLER_MISSING);
	}
}
