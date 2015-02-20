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

#include <stdarg.h>
#include <vfs.h>
#include <runtime.h>

#ifndef HAVE_PRINTF
#define PRINTF_UNAVAIABLE __attribute__((unavailable("requires compiling with printf support")))
#else
#define PRINTF_UNAVAIABLE
#endif

size_t fprintf(file_t file, const char* format, ...) PRINTF_UNAVAIABLE;
size_t vfprintf(file_t file, const char* format, va_list args) PRINTF_UNAVAIABLE;

int isdigit(int c) CONST;
size_t strlen(const char* str) PURE;

int readline(file_t file, char* buffer, size_t len);

int memcmp(const void* s1, const void* s2, size_t n) PURE;
int strcmp(const char* s1, const char* s2) PURE;
int strncmp(const char* s1, const char* s2, size_t n) PURE;
void* memcpy(void* restrict dst, const void* restrict src, size_t n);
void* memmove(void* dst, const void* src, size_t len);
void* memset(void* b, char c, size_t len);
const char* strchr(const char *s, int c);
uint32_t atoi(const char* c);
uint32_t strtol(const char* c, char **endptr, uint8_t base);
