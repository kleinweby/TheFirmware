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

#include "Firmware/Console/Console.h"

#include "Firmware/Console/CLI.h"

#include "Firmware/Schedule/Task.h"
#include "Firmware/Runtime.h"
#include "Firmware/UART.h"

#include "Firmware/Console/string.h"

namespace TheFirmware {
namespace Console {

UART stream(38400);
Schedule::Task consoleTask;
uint8_t consoleStack[1024];

void task();

void Init()
{
	consoleTask.Init((Schedule::TaskStack)consoleStack, sizeof consoleStack, task, 0);
	consoleTask.setPriority(-1);
	consoleTask.setState(Schedule::kTaskStateReady);
}

/// Checks if a given char is a digit
bool isAlpha(char c) {
	if (c >= 'a' && c <= 'z')
		return true;

	if (c >= 'A' && c <= 'Z')
		return true;
	
	return false;
}

int readline(char* buffer, size_t bufferSize)
{
	int i;

	for (i = 0; i < bufferSize - 1;) {
		char c = stream.get();

		if (c == '\r' || c == '\n') {
			buffer[i++] = '\0';
			stream.put("\r\n");
			break;
		}
		// Backspace
		else if (c == 0x7F) {
			if (i > 0) {
				i--;
				stream.put("\033[1D\033[K");
			}
		}
		// Received control sequence
		else if (c == 0x1B) {
			c = stream.get();

			// Parameter to follow
			// just read them away
			if (c == '[') {
				while (!isAlpha(c))
					c = stream.get();
			}
		}
		else {
			stream.put(c);
			buffer[i++] = c;
		}
	}

	buffer[i+1] = '\0';

	return i;
}

void task()
{
	while (1) {
		char _buffer[81];
		char* buffer = _buffer;
		size_t i = 0;

		stream.put("> ");

		i = readline(buffer, 81);

		if (i >= 80) {
			printf("Line too long.\r\n");
			continue;
		}

		char* command = strsep_ext(&buffer, " ");
		int argc = 0;

		// Calculate argument count
		{
			for (char* t = buffer; strsep_ext(&t, " "); argc++)
				;
		}

		char* argv[argc];
		// Get args
		for (int i = 0; i < argc; i++) {
			argv[i] = buffer;

			// During calculation the '\0 have been inserted, so we just need
			// to refind the boundaries now
			buffer = &buffer[strlen(buffer) + 1];
		}

		run(command, argc, argv);
	}
}

static char numberDefinitions[] = "0123456789ABCDEF";
static const char *trueString = "true";
static const char *falseString = "false";

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
		stream.put(*tempString);
	}

	return;
}

void printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	printf_va(format, args);
	va_end(args);
}

void printf_va(const char* format, va_list args)
{
	assert(format != NULL);

	while (*format != '\0') {
		// We have a format specifier here
		// so we need to evaluate it.
		if (*format == '%') {
			format++;
			
			if (*format == '%') {
				stream.put('%');
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
						stream.put('*');
					// Hex number
					case 'X':
					case 'x':
					{
						uint32_t val = va_arg(args, uint32_t);
						if (*format == 'x')
							stream.put("0x");
						printNumber(val, 16, minLength, useWhitespacePadding);
						break;
					}
					// Signed integer
					case 'd':
					case 'i':
					{
						int32_t val = va_arg(args, int32_t);
						if (val < 0) {
							stream.put('-');
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
							stream.put(trueString);
						}
						else {
							stream.put(falseString);
						}
						break;
					}
					// String
					case 's':
					{
						char* str = va_arg(args, char*);

						if (str)
							stream.put(str);
						else
							stream.put("(null)");
						break;
					}
					default:
						// Don't know the format. so we ignore it.
						break;
				}
			}
		}
		else {
			stream.put(*format);

			if (*format == '\n')
				stream.flush();
		}
		
		format++;
	}
}

} // namespace Console
} // namespace TheFirmware
