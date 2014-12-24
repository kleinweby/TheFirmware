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

#include "semaphore.h"

#include "malloc.h"
#include "runtime.h"
#include "thread.h"
#include "scheduler.h"

struct semaphore_waitee {
  list_entry_t queue_entry;
  thread_t thread;
};

semaphore_t semaphore_create(int8_t initial_value)
{
  semaphore_t semaphore = malloc_raw(sizeof(struct semaphore));

  if (!semaphore)
    return NULL;

  semaphore_init(semaphore, initial_value);

  return semaphore;
}

void semaphore_init(semaphore_t semaphore, int8_t initial_value)
{
  semaphore->value = initial_value;
  list_init(&semaphore->queue);
}

void semaphore_destory(semaphore_t semaphore)
{
  semaphore_cleanup(semaphore);
  free_raw(semaphore, sizeof(struct semaphore));
}

void semaphore_cleanup(semaphore_t semaphore)
{
  assert(list_is_empty(&semaphore->queue) == true, "Trying to clean up a semaphore with waitees");
}

void semaphore_signal(semaphore_t semaphore)
{
  semaphore->value++;

  struct semaphore_waitee* waitee = container_of(list_first(&semaphore->queue), struct semaphore_waitee, queue_entry);

  if (waitee) {
    list_delete(&semaphore->queue, list_first(&semaphore->queue));
    thread_wakeup(waitee->thread);
  }
}

void semaphore_wait(semaphore_t semaphore)
{
  struct semaphore_waitee waitee = {
    .thread = scheduler_current_thread(),
  };

  list_entry_init(&waitee.queue_entry);

  scheduler_lock();
  semaphore->value--;

  // No waiting needed
  if (semaphore->value >= 0) {
    scheduler_unlock();
    return;
  }

  list_append(&semaphore->queue, &waitee.queue_entry);
  thread_block(); // will also unlock the scheduler
}
