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

#include <clock.h>
#include <runtime.h>

herz_t clock_get_main()
{
  return gclock_get_generator(GCLOCK_GENERATOR_0) >> PM->CPUSEL.reg;
}

herz_t clock_systick_reference()
{
  return clock_get_main();
}

herz_t clock_get_source(clock_source_t source)
{
  switch (source) {
    case CLOCK_SOURCE_XOSC:
      unimplemented();
      return 0;

    case CLOCK_SOURCE_OSC8M:
      return 8000000UL >> SYSCTRL->OSC8M.bit.PRESC;

    case CLOCK_SOURCE_OSC32K:
      return 32768UL;

    case CLOCK_SOURCE_ULP32K:
      return 32768UL;

    case CLOCK_SOURCE_XOSC32K:
      unimplemented();
      return 0;

    case CLOCK_SOURCE_DFLL:
      unimplemented();
      return 0;
  };

  assert(false, "unkown source");
}

static inline void _gclock_sync()
{
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY)
    ;
}

herz_t gclock_get_generator(gclock_generator_t gen)
{
  _gclock_sync();

  /* tell the controller which clock we're talking about */
  *((uint8_t*)&GCLK->GENCTRL.reg) = gen;

  _gclock_sync();

  herz_t input = clock_get_source(GCLK->GENCTRL.bit.SRC);

  *((uint8_t*)&GCLK->GENCTRL.reg) = gen;
  _gclock_sync();
  uint8_t divsel = GCLK->GENCTRL.bit.DIVSEL;

  *((uint8_t*)&GCLK->GENCTRL.reg) = gen;
  _gclock_sync();

  uint32_t divider = GCLK->GENDIV.bit.DIV;

  if (!divsel && divider > 1) {
    input /= divider;
  }
  else if (divsel) {
    input >>= divider + 1;
  }

  return input;
}
