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

#include <list.h>
#include <arch.h>

#include <stdint.h>
#include <stdarg.h>
#include <config.h>

typedef enum {
	THREAD_STATE_UNKOWN  = 0,
	THREAD_STATE_RUNNING = 1,
	THREAD_STATE_STOPPED = 2,
	THREAD_STATE_BLOCKED = 3
} thread_state_t;

typedef struct thread* thread_t;
typedef void (*entry_func)();
typedef uint8_t tid_t;

#include <scheduler.h>

struct thread {
	list_entry_t thread_list_entry;

	tid_t tid;
	thread_state_t state;
	stack_t stack;
	stack_t stack_protector;
#if STACK_UTILISATION
	size_t stack_size;
#endif
	const char* name;

	scheduler_thread_data_t scheduler_data;
};


extern list_t* thread_list;

/// Initialize the thread subsystem
///
void thread_init();

thread_t thread_create(const char* name, size_t stack_size, stack_t stack);

void thread_set_function(thread_t thread, entry_func func, uint8_t argc, ...);
void thread_set_function_v(thread_t thread, entry_func func, uint8_t argc, va_list args);

void thread_assert_stack_protection(thread_t thread);

#if STACK_UTILISATION

void thread_stack_utilisation_reset(thread_t thread);
size_t thread_stack_utilisation(thread_t thread);

#endif

static inline void thread_set_stack(thread_t thread, stack_t stack)
{
	assert(thread->stack_protector < stack, "Stack overflow while setting the stack");
	thread->stack = stack;
}

static inline stack_t thread_get_stack(const thread_t thread)
{
	return thread->stack;
}

/// Blocks the current thread.
///
/// This call will only resume when another thread or an interrupt  woke it up
/// with thread_wakeup
///
void thread_block();

/// Stops a given thread.
///
/// Basicly the same as thread_block, but can be done to other threads
void thread_stop(thread_t thread);

/// Wakes up a given thread.
///
/// Does nothing if the thread is not blocked or stopped.
void thread_wakeup(thread_t thread);
