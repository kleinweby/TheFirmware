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

#include <log.h>

#include <printk.h>
#include <string.h>
#include <channel.h>
#include <pb_utils.h>

#if 0
#if 0
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN "\033[0;32m"
#define COLOR_GREEN "\033[0;36m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GRAY "\033[1;30m"
#else
#define COLOR_RESET ""
#define COLOR_CYAN ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_RED ""
#define COLOR_GRAY ""
#endif

void _logv(const char* file, int line, log_level_t log_level, const char* message, va_list args)
{
	const char* level_str;

	switch(log_level) {
		case LOG_LEVEL_DEBUG:
			level_str = COLOR_CYAN"[D]"COLOR_RESET;
			break;
		case LOG_LEVEL_VERBOSE:
			level_str = COLOR_CYAN"[V]"COLOR_RESET;
			break;
		case LOG_LEVEL_INFO:
			level_str = COLOR_GREEN"[I]"COLOR_RESET;
			break;
		case LOG_LEVEL_WARN:
			level_str = COLOR_YELLOW"[W]"COLOR_RESET;
			break;
		case LOG_LEVEL_ERROR:
			level_str = COLOR_RED"[E]"COLOR_RESET;
			break;
	}

	if (file) {
		fprintf(debug_serial, COLOR_GRAY"%s:%d %s ", file, line, level_str);
	}
	else {
		fprintf(debug_serial, "%s ", level_str);
	}

	vfprintf(debug_serial, message, args);
	printk("\r\n");
}

#else

#include <log.pb.h>

void _logv(const char* file, int line, log_level_t log_level, const char* message, va_list args)
{
	TheFirmware_LogMessage logMessage = TheFirmware_LogMessage_init_default;

	pb_utils_encode_vastring(&logMessage.message, message, args);

	if (file) {
		pb_utils_encode_static_string(&logMessage.file, file);
	}

	if (line > 0) {
		logMessage.has_lineNo = true;
		logMessage.lineNo = line;
	}

	// The loglevel matches the severity
	logMessage.severity = (TheFirmware_LogMessage_Severity)log_level;

	struct serial_connection conn = {
		.fd = debug_serial,
	};

	struct serial_channel chan = {
		.connection = &conn,
		.id = 0,
	};

	serial_channel_send(&chan, &TheFirmware_LogMessage_logMessage, &logMessage);
}

#endif
