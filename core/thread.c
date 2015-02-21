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

#include <thread.h>
#include <malloc.h>
#include <log.h>
#include <string.h>

list_t _thread_list;
list_t* thread_list = &_thread_list;
tid_t next_tid;

static const uint32_t kStackProtector = 0xBADBEEF;

void thread_early_init()
{
	list_init(thread_list);
	next_tid = 1;
}

thread_t thread_create(const char* name, size_t stack_size, stack_t stack)
{
	thread_t thread = malloc_raw(sizeof(struct thread));

	if (!thread)
		return NULL;

	thread_struct_init(thread, name, stack_size, stack);

	return thread;
}

void thread_struct_init(thread_t thread, const char* name, size_t stack_size, stack_t stack)
{
	memset(thread, 0, sizeof(struct thread));

	thread->state = THREAD_STATE_STOPPED;
	thread->name = name;
	thread->tid = next_tid++;
	thread->stack_protector = NULL;

	if (stack == NULL && stack_size > 0) {
		uint8_t* stack = malloc_raw(stack_size + sizeof(kStackProtector));
		thread->stack = (stack_t)&stack[stack_size + sizeof(kStackProtector) - 4];
		thread->stack_protector = (stack_t)stack;
		*(uint32_t*)thread->stack_protector = kStackProtector;

#if STACK_UTILISATION
		thread->stack_size = stack_size;
		thread_stack_utilisation_reset(thread);
#endif
	}
	else {
		thread->stack = stack;
	}

	list_entry_init(&thread->thread_list_entry);
	list_append(thread_list, &thread->thread_list_entry);

	scheduler_thread_data_init(thread);
	log(LOG_LEVEL_DEBUG, "created thread tid=%d, name=%s, stack %p", thread->tid, name, thread->stack);
}

void thread_set_function(thread_t thread, entry_func func, uint8_t argc, ...)
{
	va_list args;
	va_start(args, argc);
	thread_set_function_v(thread, func, argc, args);
	va_end(args);
}

// TODO: this is arch depended
void thread_set_function_v(thread_t thread, entry_func func, uint8_t argc, va_list args)
{
	// Prepare stack
	*thread->stack-- = (uint32_t)0x01000000L; // PSR
	*thread->stack-- = (uint32_t)func;
	*thread->stack   = (uint32_t)0xFFFFFFFEL; // ?
  thread->stack -= 5; // ?

  if (argc > 0) {
  	// Only get the first argument for now
  	*thread->stack = va_arg(args, uint32_t);
  	thread->stack -= 8;
  }
  else {
  	thread->stack -= 8;
  }
}

void thread_assert_stack_protection(thread_t thread)
{
	if (thread->stack_protector) {
		assert(thread->stack_protector < thread->stack, "Stack below stack protector");
		uint32_t* stack_protector = (uint32_t*)thread->stack_protector;
		assert(*stack_protector == kStackProtector, "Stack protector is trashed");
	}
}

#if STACK_UTILISATION

void thread_stack_utilisation_reset(thread_t thread)
{
	// We cant reset it for the current running thread at the moment
	if (thread == scheduler_current_thread())
		return;

	uint32_t* stack = thread->stack_protector;
	stack++;
	for (; stack < thread->stack; stack++)
		*stack = kStackProtector;
}

size_t thread_stack_utilisation(thread_t thread)
{
	uint32_t* stack = thread->stack_protector;
	size_t size = thread->stack_size;

	for (; stack < thread->stack && *stack == kStackProtector; stack++, size -= 4)
		;

	return size;
}

#endif

static void thread_set_state(thread_t thread, thread_state_t state)
{
	thread_state_t old_state = thread->state;
	thread->state = state;

	scheduler_thread_changed_state(thread, old_state, state);
}

void thread_block()
{
	thread_set_state(scheduler_current_thread(), THREAD_STATE_BLOCKED);
}

void thread_stop(thread_t thread)
{
	thread_set_state(thread, THREAD_STATE_STOPPED);
}

void thread_wakeup(thread_t thread)
{
	thread_set_state(thread, THREAD_STATE_RUNNING);
}
