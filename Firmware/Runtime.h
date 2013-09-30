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

/// @file Runtime.h
#pragma once

#include "stdint.h"
#include "stddef.h"

namespace TheFirmware {
/// Defines Helper Functions to interact with the runtime
namespace Runtime {

/// Handles Assertions by stopping/restarting the cpu
///
/// @param function The function in with the assertion occoured (May be NULL)
/// @param file The file in with the assertion occoured (May be NULL)
/// @param line The line in which the asseriton occoured
/// @param expr A symbolic string of the expression
/// @param msg A additional message for this expression (May be NULL)
void AssertHandler(const char* function, const char* file, uint32_t line, const char* expr, const char* msg);

/// Adapter for assertions without message
inline void AssertHandler(const char* function, const char* file, uint32_t line, const char* expr)
{
	AssertHandler(function, file, line, expr, NULL);
}

/// Initializes the runtime
///
/// @note Calls global constructors
void Init();

/// Counts variadic arguments
constexpr uint32_t CountVariadric()
{
	return 0;
}

template<typename T, typename... Args>
constexpr uint32_t CountVariadric(T value, Args... args)
{
	return CountVariadric(args...) + 1;
}

} // namespace Runtime
} // namespace TheFirmware

/// Assert a assumption in code
///
/// @param expr Expression to assert
/// @param ... message to display
#define assert(expr,...) (__builtin_expect(!(expr), 0) ? TheFirmware::Runtime::AssertHandler(__FILE__, __FUNCTION__, __LINE__, #expr, ##__VA_ARGS__) : (void)0)

/// Hint that the following code is never executed
#define Unreachable() __builtin_unreachable()

/// Denotes a unimplemented code path
#define Unimplemented() assert(false, "Unimplemented")

//
// Declares a symbol name of a type that can hold a pointer-sized integer
// with is specified by the linker.
//
#ifdef __cplusplus
#define LINKER_SYMBOL(name, type) extern "C" void _##name(); static const type name = (type)&_##name
#else
#define LINKER_SYMBOL(name, type) extern void _##name(); static const type name = (type)&_##name
#endif

#define PRINTF_FORMAT(formatIdx, vaIndex) __attribute__((__format__ (__printf__, formatIdx, vaIndex)))

