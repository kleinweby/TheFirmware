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

#include <systick.h>
#include <irq.h>
#include <clock.h>
#include <log.h>

struct systick_regs {
	volatile uint32_t CTRL;
	volatile uint32_t LOAD;
	volatile uint32_t VAL;
	const uint32_t CALIB;
};

enum {
	kCtrlCount       = (1 << 16),
	kCtrlClockSource = (1 << 2),
	kCtrlTickInt     = (1 << 1),
	kCtrlEnable      = (1 << 0),
	kCalibNoRef      = (1 << 31),
	kCalibSkew       = (1 << 30),
	kCalibTenms      = (0xFFFFFF << 0),
};

static const uint32_t kMaxLoadValue = 0xFFFFFF;

static struct systick_regs* systick_regs = (struct systick_regs*)(0xE000E000UL + 0x0010UL);

struct systick {
  struct timer timer;
  bool initialized;
};

typedef struct systick* systick_t;

static millitime_t systick_get(timer_t _timer)
{
  return systick_regs->LOAD * 1000 / clock_get_main();
}

static millitime_t systick_remaining(timer_t _timer)
{
  return systick_regs->VAL * 1000 / clock_get_main();
}

static void systick_enable(timer_t _timer)
{
  systick_regs->CTRL |= kCtrlEnable | kCtrlTickInt;
}

static void systick_disable(timer_t _timer)
{
  systick_regs->CTRL &= ~(kCtrlEnable | kCtrlTickInt);
}

static void systick_set(timer_t _timer, millitime_t time)
{
  herz_t clock = clock_get_main();

  millitime_t maxPossibleValue = 1000 * kMaxLoadValue/clock;

  if (time > maxPossibleValue)
    time = maxPossibleValue;

  systick_regs->LOAD = (clock)/1000 * time;
  systick_regs->VAL = 0;
	// log(LOG_LEVEL_INFO, "Set systick to %u", time);
  systick_enable(_timer);
}

static void systick_handle_irq(void);

static void systick_init(systick_t timer)
{
  assert(irq_register(IRQ_SYSTICK, systick_handle_irq), "Could not register systick irq");
  timer->initialized = true;
}

static const struct timer_ops systick_ops = {
  .set = systick_set,
  .get = systick_get,
  .remaining = systick_remaining,
  .enable = systick_enable,
  .disable = systick_disable,
};

static struct systick systick = {
  .timer.ops = &systick_ops,
  .initialized = false,
};

static void systick_handle_irq(void)
{
  if (systick.timer.handler)
  	systick.timer.handler(&systick.timer, systick_get(&systick.timer));
}

timer_t systick_get_timer()
{
  if (!systick.initialized) {
    systick_init(&systick);
  }
  return &systick.timer;
}
