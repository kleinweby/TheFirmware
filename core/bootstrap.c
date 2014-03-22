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

#include "bootstrap.h"

#include <arch.h>
#include <irq.h>

#include <stdint.h>

#include <test.h>
#include <log.h>
#include <printk.h>
#include <malloc.h>
#include <thread.h>
#include <scheduler.h>
#include <string.h>
#include <systick.h>

void test(timer_t timer, void* context)
{
	log(LOG_LEVEL_INFO, "Got %s :)", context);
}

void error()
{
	assert(false, "some error");
}

void bootstrap()
{
	arch_early_init();
	test_do(TEST_AFTER_ARCH_EARLY_INIT);

	irq_register(IRQ_HARDFAULT, error);

	printk_init(38400);
	irq_init();

	log(LOG_LEVEL_INFO, "Starting up TheFirmware...");

	arch_late_init();
	test_do(TEST_AFTER_ARCH_LATE_INIT);

	thread_init();
	scheduler_init();

	size_t free_mem = get_free_size();

	log(LOG_LEVEL_INFO, "Bootstrap complete. (%u free)", free_mem);

	test_do(TEST_IN_MAIN_TASK);

	timer_t timer = systick_get_timer();

	timer_managed_schedule(timer, 1000, true, test, "1 second");
	timer_managed_schedule(timer, 2000, true, test, "2 second");
	timer_managed_schedule(timer, 5000, true, test, "5 second");

	// while (1) {
	// 	char buf[60];
	// 	char* str = "> ";
	// 	write(debug_serial, str, strlen(str));
	//
	// 	if (readline(debug_serial, buf, 60) < 0) {
	// 		str = "error reading\r\n";
	// 		write(debug_serial, str, strlen(str));
	// 	}
	// 	else {
	// 		str = "got it!\r\n";
	// 		write(debug_serial, str, strlen(str));
	// 	}
	// }

	while(1)
		; //__asm("WFI");
}