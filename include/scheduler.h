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

#pragma once

#include <stdint.h>
#include <arch.h>
#include <runtime.h>
#include <list.h>
#include <timer.h>

/// Initializes the scheduler and transforms the current context into the main
/// thread.
void scheduler_init();

/// Takes a stack for the current thread, saves it and returns the stack of the
/// new thread to run.
///
/// Should be called by the arch primitive which is called by arch_yield.
///
/// @Note to force a rescheduling, call yield.
///
stack_t schedule(stack_t stack);

/// Lock access to the scheduler
static ALWAYS_INLINE void scheduler_lock() {
  arch_disable_irqs();
}

/// Unlock access to the scheduler
static ALWAYS_INLINE void scheduler_unlock() {
  arch_enable_irqs();
}

// Yield control to the scheduler which may give another thread a change to run
//
void yield() ALIAS(arch_yield);

extern uint8_t _in_isr_count;

// Returns true when the current path is executed in an isr.
//
// This is used to determine if must use busy waiting, as we can't block the current
// thread.
//
static ALWAYS_INLINE bool scheduler_in_isr() {
	return _in_isr_count > 0;
}

static ALWAYS_INLINE void scheduler_enter_isr() {
	_in_isr_count++;
}

static ALWAYS_INLINE void scheduler_leave_isr() {
	_in_isr_count--;
}

struct scheduler_thread_data {
  list_entry_t queue_entry;
};
typedef struct scheduler_thread_data scheduler_thread_data_t;

void delay(millitime_t time);

#include <thread.h>

/// Get the current running thread
thread_t scheduler_current_thread();

/// Initializes the scheduler_thread_data for a new thread
///
void scheduler_thread_data_init(thread_t thread);

//
// Callback functions to let the scheduler know changed to a thread
//

void scheduler_thread_changed_state(thread_t thread, thread_state_t old_state, thread_state_t new_state);

void scheduler_set_idle_thread(thread_t thread);

