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

///
/// TODO: there are many possible cases where a proper firering of a timer may be
/// delayed
///

#include <timer.h>
#include <malloc.h>
#include <scheduler.h>
#include <log.h>

static void timer_managedhandler(timer_t timer, millitime_t elapsed_time);

void timer_set(timer_t timer, millitime_t time)
{
  assert(time > 0, "Setting a timers time to a negative ammount makes no sense");

  timer->ops->set(timer, time);
}

millitime_t timer_get(timer_t timer)
{
  return timer->ops->get(timer);
}

millitime_t timer_remaining(timer_t timer)
{
  return timer->ops->remaining(timer);
}

void timer_enable(timer_t timer)
{
  timer->ops->enable(timer);
}

void timer_disable(timer_t timer)
{
  timer->ops->disable(timer);
}

void timer_set_handler(timer_t timer, timer_handler_t handler)
{
  assert(timer->handler != timer_managedhandler, "Attempt to change timer handler while it is beeing managed");

  timer->handler = handler;
}

timer_handler_t timer_get_handler(timer_t timer)
{
  return timer->handler;
}

struct timer_managed_timeout {
  list_entry_t entry;
  millitime_t remaining;
  millitime_t reset_time;
  timer_managedhandler_t handler;
  void* context;
};

static void timer_managed_recalculate(timer_t timer)
{
  struct timer_managed_timeout* t = container_of(list_first(&timer->managed_timeouts), struct timer_managed_timeout, entry);

  if (t)
    timer_set(timer, t->remaining);
  else
    timer_disable(timer);
}

static void timer_managed_update_first(timer_t timer)
{
  // struct timer_managed_timeout* t = container_of(list_first(&timer->managed_timeouts), struct timer_managed_timeout, entry);
  //
  // if (t) {
  //   log(LOG_LEVEL_INFO, "get %u remaining %u", timer_get(timer), timer_remaining(timer));
  //   millitime_t elapsed = timer_get(timer) - timer_remaining(timer);
  //   t->remaining -= elapsed;
  //   timer_set(timer, t->remaining);
  // }
}

static void timer_managed_insert(timer_t timer, struct timer_managed_timeout* timeout)
{
  assert(timer, "Timer can not be NULL");
  assert(timeout, "Timeout can not be NULL");

  if (list_is_empty(&timer->managed_timeouts)) {
    // log(LOG_LEVEL_INFO, "Insert %p with remaining %u", timeout, timeout->remaining);
    list_append(&timer->managed_timeouts, &timeout->entry);
    timer_managed_recalculate(timer);
    return;
  }

  timer_managed_update_first(timer);

  struct timer_managed_timeout* t;
  list_foreach_contained(t, &timer->managed_timeouts, struct timer_managed_timeout, entry) {
    if (timeout->remaining <= t->remaining) {
      if (&t->entry == list_first(&timer->managed_timeouts)) {
        list_insert_before(&t->entry, &timeout->entry);
        list_rrotate(&timer->managed_timeouts);

        timer_managed_recalculate(timer);
      }
      else {
        list_insert_before(&t->entry, &timeout->entry);
      }
      t->remaining -= timeout->remaining;
      return;
    }
    timeout->remaining -= t->remaining;
  }

  list_append(&timer->managed_timeouts, &timeout->entry);
}

void timer_managed_schedule(timer_t timer, millitime_t timeout, bool repeat, timer_managedhandler_t handler, void* context)
{
  assert(timer, "Timer can not be NULL");
  assert(timeout > 0, "Timeout can not be negative");
  assert(handler, "Timeout requires a handler");

  scheduler_lock();
  if (timer_get_handler(timer) != timer_managedhandler) {
    list_init(&timer->managed_timeouts);
    timer_set_handler(timer, timer_managedhandler);
  }

  struct timer_managed_timeout *managed_timeout = malloc_raw(sizeof(struct timer_managed_timeout));

  list_entry_init(&managed_timeout->entry);
  managed_timeout->remaining = timeout;
  managed_timeout->reset_time = repeat ? timeout : 0;
  managed_timeout->handler = handler;
  managed_timeout->context = context;

  timer_managed_insert(timer, managed_timeout);
  scheduler_unlock();
}

void timer_managed_cancel(timer_t timer, timer_managedhandler_t handler, void* context)
{
  assert(timer, "Timer can not be NULL");
  assert(handler, "Handler can not be NULL");

  scheduler_lock();

  struct timer_managed_timeout* t;
  list_foreach_contained(t, &timer->managed_timeouts, struct timer_managed_timeout, entry) {
    if (t->handler == handler && t->context == context) {
      bool first = false;

      if (&t->entry == list_first(&timer->managed_timeouts)) {
        first = true;
        timer_managed_update_first(timer);
      }

      struct timer_managed_timeout* next = container_of(list_next(&timer->managed_timeouts, &t->entry), struct timer_managed_timeout, entry);

      if (next) {
        next->remaining += t->remaining;
      }

      list_delete(&timer->managed_timeouts, &t->entry);
      free_raw(t, sizeof(struct timer_managed_timeout));

      if (first)
        timer_managed_recalculate(timer);
      break;
    }
  }

  scheduler_unlock();
}

static void timer_managedhandler(timer_t timer, millitime_t elapsed_time)
{
  assert(timer, "Timer can not be NULL");

  scheduler_lock();
  struct timer_managed_timeout* t = container_of(list_first(&timer->managed_timeouts), struct timer_managed_timeout, entry);

  if (t) {
    t->remaining -= elapsed_time;
    scheduler_unlock();

    while (t->remaining <= 0) {
      scheduler_lock();
      bool free_it = true;
      list_delete(&timer->managed_timeouts, &t->entry);

      if (t->reset_time > 0) {
        t->remaining = t->reset_time;
        timer_managed_insert(timer, t);
        free_it = false;
      }

      scheduler_unlock();
      t->handler(timer, t->context);
      if (free_it)
        free_raw(t, sizeof(struct timer_managed_timeout));

      t = container_of(list_first(&timer->managed_timeouts), struct timer_managed_timeout, entry);
    }

    timer_managed_recalculate(timer);
  }
  else {
    scheduler_unlock();
  }
}
