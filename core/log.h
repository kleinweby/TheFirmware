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
#include <config.h>

typedef enum {
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_VERBOSE = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_WARN = 3,
	LOG_LEVEL_ERROR = 4,
} log_level_t;

static const log_level_t min_log_level = LOG_LEVEL_VERBOSE;

void _logv(const char* file, int line, log_level_t log_level, const char* message, va_list args);

static inline void _log(const char* file, int line, log_level_t log_level, const char* message, ...) {
	if (log_level >= min_log_level) {
		va_list args;
		va_start(args, message);
		_logv(file, line, log_level, message, args);
		va_end(args);
	}
}

#if LOG_SOURCE_LOCATION
#define log(log_level, message, ...) _log(__FILE__, __LINE__, log_level, message, #__VA_ARGS__ )
#else
#define log(log_level, message, ...) _log(NULL, 0, log_level, message, #__VA_ARGS__ )
#endif //LOG_SOURCE_LOCATION
