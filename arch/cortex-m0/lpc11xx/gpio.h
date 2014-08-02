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

typedef uint32_t pin_t;

#define PIN_NUMBER_2_6 1
#define PIN_NUMBER_2_0 2
#define PIN_NUMBER_0_0 3
#define PIN_NUMBER_0_1 4
#define PIN_NUMBER_1_8 9
#define PIN_NUMBER_0_2 10
#define PIN_NUMBER_2_7 11
#define PIN_NUMBER_2_8 12
#define PIN_NUMBER_2_1 13
#define PIN_NUMBER_0_3 14
#define PIN_NUMBER_0_4 15
#define PIN_NUMBER_0_5 16
#define PIN_NUMBER_1_9 17
#define PIN_NUMBER_2_4 18
#define PIN_NUMBER_2_5 21
#define PIN_NUMBER_0_6 22
#define PIN_NUMBER_0_7 23
#define PIN_NUMBER_2_9 24
#define PIN_NUMBER_2_10 25
#define PIN_NUMBER_2_2 26
#define PIN_NUMBER_0_8 27
#define PIN_NUMBER_0_9 28
#define PIN_NUMBER_0_10 29
#define PIN_NUMBER_1_10 30
#define PIN_NUMBER_2_11 31
#define PIN_NUMBER_0_11 32
#define PIN_NUMBER_1_0 33
#define PIN_NUMBER_1_1 34
#define PIN_NUMBER_1_2 35
#define PIN_NUMBER_3_0 36
#define PIN_NUMBER_3_1 37
#define PIN_NUMBER_2_3 38
#define PIN_NUMBER_1_3 39
#define PIN_NUMBER_1_4 40
#define PIN_NUMBER_1_11 42
#define PIN_NUMBER_3_2 43
#define PIN_NUMBER_1_5 45
#define PIN_NUMBER_1_6 46
#define PIN_NUMBER_1_7 47
#define PIN_NUMBER_3_3 48

#define _PIN_NUM2(base, pin) base##pin
#define _PIN_NUM1(port, pin) _PIN_NUM2(PIN_NUMBER_##port##_, pin)

#define PIN(port, pin) ((pin_t)(((port) << 24) | (pin) << 8) | _PIN_NUM1(port, pin) )

#include_next <gpio.h>