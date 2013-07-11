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

#include "Log.h"
#include "Firmware/Target/LPC11xx/UART.h"
#include "Firmware/Runtime.h"

#include <stdint.h>

namespace TheFirmware {
namespace Log {

static char numberDefinitions[] = "0123456789ABCDEF";
static const char *trueString = "true";
static const char *falseString = "false";

using LogStream = LPC11xx::UART;

LogStream logStream(38400);

/// Checks if a given char is a digit
bool isDigit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	
	return false;
}

/// Prints a given number
///
/// @param number The Number to print
/// @param base The base to print the number in
/// @param minLength Min length which wil be padded
/// @param useWhitespacePadding Pad with whitespace instead of zeros
void printNumber(uint32_t number, uint8_t base, uint32_t minLength, bool useWhitespacePadding)
{		
	// We only support base 2 to 16
	if (base < 2 || base > 16)
		return;
	
	char buffer[25];
	char* tempString = buffer;
	uint32_t usedLength = 0;
	
	// First we put the number in in reverse to
	// calculate the needed space and the number
	// itself
	do {
		uint8_t remainder;
		
		remainder = number % base;
		number = number / base;
		
		*tempString = numberDefinitions[remainder];
		
		usedLength++;
		tempString++;
	} while(number > 0);
	
	// Now we know the needed length and
	// have the number in a reverse format aviaible
	
	// Now padd the thing.
	for (; usedLength < minLength; usedLength++) {
		if (useWhitespacePadding)
			*tempString = ' ';
		else
			*tempString = '0';
		tempString++;
	}
	
	tempString--;
	
	// Now swap the thing around
	for (uint8_t i = 0; i < usedLength; i++, tempString--) {
		logStream.put(*tempString);
	}

	return;
}

void Logv(LogLevel logLevel, const char* format, va_list args)
{
	assert(format != NULL);
	
	// Print level
	switch(logLevel) {
		case kLogLevelError:
			logStream.put("\033[1;31m[E]\033[0m ");
			break;
		case kLogLevelWarn:
			logStream.put("\033[1;33m[W]\033[0m ");
			break;
		case kLogLevelInfo:
			logStream.put("\033[0;34m[I]\033[0m ");
			break;
		case kLogLevelVerbose:
			logStream.put("\033[0;32m[V]\033[0m ");
			break;
		case kLogLevelDebug:
			logStream.put("\033[0;32m[D]\033[0m ");
			break;
	}

	while (*format != '\0') {
		// We have a format specifier here
		// so we need to evaluate it.
		if (*format == '%') {
			format++;
			
			if (*format == '%') {
				logStream.put('%');
			}
			// It is in deed an format
			else {
				bool useWhitespacePadding = true;
				uint32_t minLength = 0;
				int32_t precision = -1;
				
				// Read the length specifier
				// if there is any.
				while (isDigit(*format)) {
					// An leading 0 means we should padd with
					// 0 instand of whitespaces (if it is an numeric
					// format)
					if (minLength == 0 && *format == '0') {
						useWhitespacePadding = false;
					} else {
						minLength = minLength * 10 + ((uint32_t)*format - '0');
					}
					format++;
				}
				
				// The precision is specified
				if (*format == '.') {
					format++;
					precision = 0;
					
					while (isDigit(*format)) {
						precision = precision * 10 + (*format - '0');
						format++;
					}
				}
				
				// Now we've parsed away the formatting options
				// let look at the format
				switch(*format) {
					// Pointer
					case 'p':
						minLength = 8;
						useWhitespacePadding = false;
						logStream.put('*');
					// Hex number
					case 'X':
					case 'x':
					{
						uint32_t val = va_arg(args, uint32_t);
						logStream.put("0x");
						printNumber(val, 16, minLength, useWhitespacePadding);
						break;
					}
					// Signed integer
					case 'd':
					case 'i':
					{
						int32_t val = va_arg(args, int32_t);
						if (val < 0) {
							logStream.put('-');
							val = -val;
						}
						
						printNumber((uint32_t)val, 10, minLength, useWhitespacePadding);
						break;
					}
					// Unsinged integer
					case 'u':
					{
						uint32_t val = va_arg(args, uint32_t);
						printNumber(val, 10, minLength, useWhitespacePadding);
						break;
					}
					// Boolean
					case 'B':
					{
						uint32_t val = va_arg(args, uint32_t);
						
						if (val == true) {
							logStream.put(trueString);
						}
						else {
							logStream.put(falseString);
						}
						break;
					}
					// String
					case 's':
					{
						char* str = va_arg(args, char*);

						logStream.put(str);
						break;
					}
					default:
						// Don't know the format. so we ignore it.
						break;
				}
			}
		}
		else {
			logStream.put(*format);
		}
		
		format++;
	}

	logStream.put("\r\n");
	logStream.flush();
}

} // namespace Log
} // namespace TheFirmware
