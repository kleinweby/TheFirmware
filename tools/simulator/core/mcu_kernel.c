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

#include "mcu_kernel.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Target.h>

struct mcu_kernel {
  LLVMModuleRef module;
  LLVMExecutionEngineRef engine;

  void (*execute)();
  void (*disassemble)(void (*print)(char*,...));
  const char* (*reg2str)(uint8_t reg);
  uint32_t (*read_reg)(uint8_t reg);
  void (*write_reg)(uint8_t reg, uint32_t val);
};

struct func_desc {
  char* name;
  int offset;
  bool required;
} func_descs[] = {
#define FUNC(_name, _required) {.name = "kernel." # _name, .offset = offsetof(struct mcu_kernel, _name), .required = _required }
  FUNC(execute, true),
  FUNC(disassemble, true),
  FUNC(reg2str, true),
  FUNC(read_reg, true),
  FUNC(write_reg, true),
#undef FUNC
  { NULL }
};

struct callback_desc {
  char* name;
  int offset;
} callback_descs[] = {
#define CALLBACK(_name) {.name = "callback."#_name, .offset = offsetof(struct mcu_kernel_callbacks, _name)}
  CALLBACK(fetch8),
  CALLBACK(fetch16),
  CALLBACK(fetch32),
  CALLBACK(write8),
  CALLBACK(write16),
  CALLBACK(write32),
#undef CALLBACK
  { NULL }
};

void mcu_kernel_init()
{
  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();
}

mcu_kernel_t mcu_kernel_create(const char* filename, struct mcu_kernel_callbacks callbacks)
{
  char* error;
  mcu_kernel_t kernel = calloc(1, sizeof(struct mcu_kernel));
  LLVMContextRef context = LLVMGetGlobalContext();
  LLVMMemoryBufferRef memBuff;

  if (!kernel) {
    perror("Could not alloc kernel struct");
    return NULL;
  }

  if (LLVMCreateMemoryBufferWithContentsOfFile(filename, &memBuff, &error)) {
    printf("Could not open kernel file: %s\n", error);
    return NULL;
  }

  if (LLVMParseIRInContext(context, memBuff, &kernel->module, &error)) {
    printf("Could not parse kernel: %s\n", error);
    return NULL;
  }

  if (LLVMCreateExecutionEngineForModule(&kernel->engine, kernel->module, &error)) {
    printf("Error creating execution engine: %s\n", error);
    return NULL;
  }

  {
    LLVMValueRef ref = LLVMGetNamedGlobal(kernel->module, "callback.context");

    if (!ref) {
      printf("kernel does not accept a callback context\n");
      return NULL;
    }

    LLVMAddGlobalMapping(kernel->engine, ref, callbacks.context);
  }

  for (struct callback_desc* desc = callback_descs; desc->name != NULL; desc++) {
    LLVMValueRef ref;

    ref = LLVMGetNamedFunction(kernel->module, desc->name);

    if (!ref) {
      // printf("Callback %s skipped. Not needed by kernel.\n", desc->name);
      continue;
    }

    LLVMAddGlobalMapping(kernel->engine, ref, *(void**)(((intptr_t)&callbacks) + desc->offset));
  }

  for (struct func_desc* desc = func_descs; desc->name != NULL; desc++) {
    LLVMValueRef func;
    if (LLVMFindFunction(kernel->engine, desc->name, &func)) {
      printf("Did not find function with name %s\n", desc->name);

      if (desc->required)
        return NULL;
      else
        continue;
    }

    void* funcAddr = LLVMGetPointerToGlobal(kernel->engine, func);

    *((void**)(((intptr_t)kernel) + desc->offset)) = funcAddr;
  }

  return kernel;
}

bool mcu_kernel_disassemble(mcu_kernel_t kernel, void (*print)(const char* fmt, ...))
{
  kernel->disassemble(print);
  return true;
}

bool mcu_kernel_execute(mcu_kernel_t kernel)
{
  kernel->execute();
  return true;
}

uint32_t mcu_kernel_read_reg(mcu_kernel_t kernel, uint8_t reg_num)
{
  return kernel->read_reg(reg_num);
}

void mcu_kernel_write_reg(mcu_kernel_t kernel, uint8_t reg_num, uint32_t value)
{
  kernel->write_reg(reg_num, value);
}
