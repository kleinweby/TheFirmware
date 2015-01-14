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

#include <scheduler.h>
#include <thread.h>
#include <systick.h>

static struct {
  thread_t current_thread;
  thread_t idle_thread;
  list_t running_queue;
} scheduler;

uint8_t _in_isr_count = 0;

void scheduler_init()
{
  list_init(&scheduler.running_queue);

  scheduler.current_thread = thread_create("main", 0, NULL);
  thread_wakeup(scheduler.current_thread);
  yield();
}

stack_t schedule(stack_t stack)
{
  scheduler_enter_isr();
  thread_set_stack(scheduler.current_thread, stack);

#ifdef STACK_CHECK_PROTECTOR
  thread_assert_stack_protection(scheduler.current_thread);
#endif

  scheduler_lock();
  if (list_is_empty(&scheduler.running_queue)) {
    scheduler.current_thread = scheduler.idle_thread;
  }
  else {
    list_lrotate(&scheduler.running_queue);
    scheduler.current_thread = container_of(list_first(&scheduler.running_queue), struct thread, scheduler_data.queue_entry);
  }
  scheduler_unlock();

  stack = thread_get_stack(scheduler.current_thread);
  scheduler_leave_isr();
  return stack;
}

thread_t scheduler_current_thread()
{
  return scheduler.current_thread;
}

static void delay_handler(timer_t timer, void* context)
{
  thread_wakeup(context);

  // If we're idling, yield now
  if (scheduler.current_thread == scheduler.idle_thread)
    yield();
}

void delay(millitime_t time)
{
  scheduler_lock();
  timer_managed_schedule(systick_get_timer(), time, false, delay_handler, scheduler_current_thread());
  thread_block();
  scheduler_unlock();
}

void scheduler_thread_data_init(thread_t thread)
{
  list_entry_init(&thread->scheduler_data.queue_entry);
}

void scheduler_thread_changed_state(thread_t thread, thread_state_t old_state, thread_state_t new_state)
{
  scheduler_lock();

  if (old_state == THREAD_STATE_RUNNING && new_state != THREAD_STATE_RUNNING) {
    list_delete(&scheduler.running_queue, &thread->scheduler_data.queue_entry);

    // The current thread is no longer running, reschedule is mandetory
    if (thread == scheduler.current_thread)
      yield();
  }
  else if (old_state != THREAD_STATE_RUNNING && new_state == THREAD_STATE_RUNNING) {
    list_append(&scheduler.running_queue, &thread->scheduler_data.queue_entry);
  }

  // if we're currently idily always try to reschudle
  if (scheduler.current_thread == scheduler.idle_thread)
    yield();

  scheduler_unlock();
}

void scheduler_set_idle_thread(thread_t thread)
{
  scheduler.idle_thread = thread;
}
