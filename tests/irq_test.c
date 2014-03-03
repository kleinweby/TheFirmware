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

#include <test.h>
#include <irq.h>

#ifdef TESTS_SUPPORTED

static bool handler_called = false;

static void irq_handler() {
	handler_called = true;
}

static void test_irq_register_handler() {
	test_assert(irq_register(IRQ0, irq_handler) == true, "IRQ handler registraion faild");
}

DECLARE_TEST("register irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_register_handler);

static void test_irq_register_unkown_handler() {
	test_assert(irq_register(NUMBER_OF_IRQS, irq_handler) == false, "Could register irq=NUMBER_OF_IRQS");
}

DECLARE_TEST("register unkown irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_register_unkown_handler);

static void test_irq_handler_called() {
	test_assert(irq_register(IRQ0, irq_handler) == true, "IRQ handler registraion faild");

	handler_called = false;

	do_irq(IRQ0);

	test_assert(handler_called == true, "IRQ handler was not called");
}
DECLARE_TEST("call irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_handler_called);

static void test_irq_unregister_handler() {
	test_assert(irq_register(IRQ0, irq_handler) == true, "IRQ handler registraion faild");

	test_assert(irq_unregister(IRQ0, irq_handler) == true, "IRQ handler unregistraion faild");
}

DECLARE_TEST("unregister irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_unregister_handler);

static void test_irq_unregister_unkown_handler() {
	test_assert(irq_unregister(IRQ1, irq_handler) == false, "IRQ handler unregistraion should not succeed");
	test_assert(irq_unregister(NUMBER_OF_IRQS, irq_handler) == false, "NUMBER_OF_IRQS IRQ handler unregistraion should not succeed");
}

DECLARE_TEST("unregister unkown irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_unregister_unkown_handler);

static void test_irq_missing_handler() {
	test_assert(irq_register(IRQ_HANDLER_MISSING, irq_handler) == true, "IRQ handler registraion faild");

	handler_called = false;

	do_irq(IRQ0);

	test_assert(handler_called == true, "missing IRQ handler was not called");
}

DECLARE_TEST("missing irq handler", TEST_AFTER_ARCH_LATE_INIT, test_irq_missing_handler);

#endif // TESTS_SUPPORTED
