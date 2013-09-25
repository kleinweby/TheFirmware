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
#include <stdint.h>

namespace TheFirmware {
namespace Console {

/// Describes the servity of the log message
typedef enum : uint8_t {
	kLogLevelDebug,
	kLogLevelVerbose,
	kLogLevelInfo,
	kLogLevelWarn,
	kLogLevelError
} LogLevel;

/// Prints the given message
///
/// @param logLevel the severity of the message
/// @param format format string to display
/// @param args variable arguments for the format string
/// @note Please us the defined helper methods
void Logv(LogLevel logLevel, const char* format, va_list args);

/// @defgroup LogHelper Convenience Functions for Logging
/// Use this macros when you what to log something
/// @{

/// Prints a debug message
static inline void LogDebug(const char* format, ...) {
	va_list args;
	va_start(args, format);
	Logv(kLogLevelDebug, format, args);
	va_end(args);
}

/// Prints a verbose message
static inline void LogVerbose(const char* format, ...) {
	va_list args;
	va_start(args, format);
	Logv(kLogLevelVerbose, format, args);
	va_end(args);
}

/// Prints a info message
static inline void LogInfo(const char* format, ...) {
	va_list args;
	va_start(args, format);
	Logv(kLogLevelInfo, format, args);
	va_end(args);
}

/// Prints a warn message
static inline void LogWarn(const char* format, ...) {
	va_list args;
	va_start(args, format);
	Logv(kLogLevelWarn, format, args);
	va_end(args);
}

/// Prints a error message
static inline void LogError(const char* format, ...) {
	va_list args;
	va_start(args, format);
	Logv(kLogLevelError, format, args);
	va_end(args);
}

/// @}

} // namespace Console
} // namespace TheFirmware
