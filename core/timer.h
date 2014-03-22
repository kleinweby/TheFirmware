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
/// @file timer.h
/// @defgroup timer Timer
/// @{

#pragma once

#include <stdint.h>
#include <list.h>
#include <runtime.h>

typedef struct timer* timer_t;
// Describes the time in miliseconds
// will overflow approx. every 24days
typedef int32_t millitime_t;

typedef void (*timer_handler_t)(timer_t timer, millitime_t elapsed_time);
typedef void (*timer_managedhandler_t)(timer_t timer, void* context);

/// Struct containg all operations on a timer
/// @internal
struct timer_ops {
  /// @see timer_set(timer_t timer, militime_t)
  void (*set)(timer_t timer, millitime_t time);

  /// @see timer_get(timer_t timer)
  millitime_t (*get)(timer_t timer);

  /// @see timer_remaining(timer_t timer)
  millitime_t (*remaining)(timer_t timter);

  /// @see timer_enable(timer_t timer)
  void (*enable)(timer_t timer);

  /// @see timer_disable(timer_t timer)
  void (*disable)(timer_t timer);
};

/// Struct descriping a timer
/// @internal
struct timer {
  const struct timer_ops* ops;
  timer_handler_t handler;
  list_t managed_timeouts;
};

/// Sets the time in with the timer should fire
///
/// @note Setting the firetime will enable the timer.
///
/// @note A timer may not support all of the millitime_t range, if so
/// the timer will clamp the specified time into its capabilities. Therefore
/// you need to always check the actual time elapsed.
///
/// @param timer Timer to set the fire time
/// @param time time in miliseconds in which the timer should fire
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

/// Schedules a call to timer_handler after a specified timeout.
///
/// Puts the timer in the managed mode if not already.
///
/// @param timer to operate on
/// @param timeout time after which the handler should be called
/// @param repeat true if the timer_handler should be called repeatilly
/// @param handler handler to call
/// @param context context passed to the handler
void timer_managed_schedule(timer_t timer, millitime_t timeout, bool repeat, timer_managedhandler_t handler, void* context);

/// Removes a given timer.
///
/// @param timer Timer to operate on
/// @param handler Handler to remove
/// @param context Context it was registered with
void timer_managed_cancel(timer_t timer, timer_managedhandler_t handler, void* context);

/// @}
