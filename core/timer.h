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
#include <runtime.h>

typedef struct timer* timer_t;
// Describes the time in miliseconds
// will overflow approx. every 24days
typedef int32_t millitime_t;

typedef void (*timer_handler_t)(millitime_t elapsed_time);

struct timer_ops {
  void (*set)(timer_t timer, millitime_t time);
  millitime_t (*get)(timer_t timer);
  millitime_t (*remaining)(timer_t timter);

  void (*enable)(timer_t timer);
  void (*disable)(timer_t timer);
};

struct timer {
  const struct timer_ops* ops;
  timer_handler_t handler;
};

static inline void timer_set(timer_t timer, millitime_t time)
{
  assert(time > 0, "Setting a timers time to a negative ammount makes no sense");

  timer->ops->set(timer, time);
}

static inline millitime_t timer_get(timer_t timer)
{
  return timer->ops->get(timer);
}

static inline millitime_t timer_remaining(timer_t timer)
{
  return timer->ops->remaining(timer);
}

static inline void timer_enable(timer_t timer)
{
  timer->ops->enable(timer);
}

static inline void timer_disable(timer_t timer)
{
  timer->ops->disable(timer);
}

void timer_set_handler(timer_t timer, timer_handler_t handler);
timer_handler_t timer_get_handler(timer_t timer);
