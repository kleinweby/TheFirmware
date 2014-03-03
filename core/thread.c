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

list_t _thread_list;
list_t* thread_list = &_thread_list;

void thread_init()
{
	list_init(thread_list);
}

thread_t thread_create(const char* name, size_t stack_size, stack_t stack)
{
	thread_t thread = malloc_raw(sizeof(struct thread));

	if (!thread)
		return NULL;

	thread->state = THREAD_STATE_STOPPED;
	thread->name = name;

	list_append(thread_list, &thread->thread_list_entry);
	log(LOG_LEVEL_DEBUG, "created thread %s", name);

	return thread;
}

void thread_set_function(thread_t thread, entry_func func, uint8_t argc, ...)
{
	va_list args;
	va_start(args, argc);
	thread_set_function_v(thread, func, argc, args);
	va_end(args);
}

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
