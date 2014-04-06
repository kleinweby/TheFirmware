//
// Copyright (c) 2014, Christian Speich
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
#include <stdbool.h>

typedef struct mcu_kernel* mcu_kernel_t;
struct mcu_kernel_callbacks {
  void* context;

  bool (*fetch8)(void* context, uint32_t addr, uint8_t* val);
  bool (*fetch16)(void* context, uint32_t addr, uint16_t* val);
  bool (*fetch32)(void* context, uint32_t addr, uint32_t* val);

  bool (*write8)(void* context, uint32_t addr, uint8_t* val);
  bool (*write16)(void* context, uint32_t addr, uint16_t val);
  bool (*write32)(void* context, uint32_t addr, uint32_t val);
};

void mcu_kernel_init();

mcu_kernel_t mcu_kernel_create(const char* filename, struct mcu_kernel_callbacks callbacks);

bool mcu_kernel_disassemble(mcu_kernel_t kernel, void (*)(const char* fmt, ...));
bool mcu_kernel_execute(mcu_kernel_t kernel);

uint32_t mcu_kernel_read_reg(mcu_kernel_t kernel, uint8_t reg_num);
void mcu_kernel_write_reg(mcu_kernel_t kernel, uint8_t reg_num, uint32_t value);
