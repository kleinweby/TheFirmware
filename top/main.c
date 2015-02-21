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

#include <scheduler.h>
#include <arch.h>
#include <platform.h>
#include <target.h>
#include <log.h>
#include <console.h>
#include <platform/printk.h>
#include <platform/irq.h>
#include <fw/init.h>

void firmware_main()
{
	scheduler_enter_isr();
	fw_init_run(kFWInitLevelEarliest);

	thread_early_init();
	arch_early_init();
	irq_init();

	platform_early_init();

	target_early_init();

	log(LOG_LEVEL_INFO, "Starting up TheFirmware...");

	// thread_init();
	scheduler_init();
	fw_init_run(kFWInitLevelThreading);

	scheduler_leave_isr();

	arch_init();
	platform_init();
	target_init();

	console_spawn(debug_serial);
	
	fw_init_run(kFWInitLevelLast);
	target_main();
	thread_block();
	for (;;);
}