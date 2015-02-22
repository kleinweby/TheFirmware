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

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#define NO_RETURN  __attribute__ ((noreturn))
#define ALWAYS_INLINE __attribute__((always_inline))

/// Handles Assertions by stopping/restarting the cpu
///
/// @param function The function in with the assertion occoured (May be NULL)
/// @param file The file in with the assertion occoured (May be NULL)
/// @param line The line in which the asseriton occoured
/// @param expr A symbolic string of the expression
/// @param msg A additional message for this expression (May be NULL)
void _assert_handler(const char* function, const char* file, uint32_t line, const char* expr, const char* msg) NO_RETURN;

/// Assert a assumption in code
///
/// @param expr Expression to assert
/// @param ... message to display
#define assert(expr,...) (__builtin_expect(!(expr), 0) ? _assert_handler(__FUNCTION__, __FILE__, __LINE__, #expr, ##__VA_ARGS__) : (void)0)

/// Hint that the following code is never executed
#define unreachable() __builtin_unreachable()

/// Denotes a unimplemented code path
#define unimplemented() assert(false, "Unimplemented")

#define CONST __attribute__((const))
#define PURE __attribute__((pure))

#define NONNULL(...) __attribute__((nonnull (__VA_ARGS__)))

#define OFFSET_PTR(ptr, offset) ((void*)((uintptr_t)ptr + offset))

#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((alias (#f)))
#define LINKER_SYMBOL(name, type) extern void _##name(); static const type name = (type)&_##name

#define ENUM(_type, _name) _type _name; enum

#define container_of(ptr, type, member) (ptr ? ({                  \
  const __typeof( ((type *)0)->member ) *__mptr = (ptr);    \
  (type *)( (char *)__mptr - offsetof(type,member) );}) : NULL)

#define STR(s) #s
#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a,b)
#define GEN_SECTION_NAME(ns, name) ".text.gen."#ns"."#name
#define GEN_SECTION_ATTR(ns, name) __attribute__ ((section (GEN_SECTION_NAME(ns, name))))

typedef uint32_t off_t;

typedef uint32_t status_t;

#define STATUS_OK 0
#define STATUS_ERR(x) (((x) << 1) | 1)
#define STATUS_NOT_SUPPORTED STATUS_ERR(1)

