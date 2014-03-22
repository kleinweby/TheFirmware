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

#include_next <clock.h>

#include "samd20.h"

typedef enum {
  /// Internal 8MHz RC oscillator
  CLOCK_SOURCE_OSC8M    = GCLK_SOURCE_OSC8M,
  /// Internal 32kHz RC oscillator
  CLOCK_SOURCE_OSC32K   = GCLK_SOURCE_OSC32K,
  /// External oscillator
  CLOCK_SOURCE_XOSC     = GCLK_SOURCE_XOSC ,
  /// External 32kHz oscillator/
  CLOCK_SOURCE_XOSC32K  = GCLK_SOURCE_XOSC32K,
  /// Digital Frequency Locked Loop (DFLL)
  CLOCK_SOURCE_DFLL     = GCLK_SOURCE_DFLL48M,
  /// Internal Ultra Low Power 32kHz oscillator
  CLOCK_SOURCE_ULP32K   = GCLK_SOURCE_OSCULP32K,
} clock_source_t;

herz_t clock_get_source(clock_source_t source);

///
/// Generic Clock Controller
///

typedef enum {
  GCLOCK_GENERATOR_0,
} gclock_generator_t;

herz_t gclock_get_generator(gclock_generator_t gen);
