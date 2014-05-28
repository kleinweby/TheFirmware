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

#include <string.h>

#include <runtime.h>

#include <stdint.h>
#include <stdbool.h>

static const char numberDefinitions[] = "0123456789ABCDEF";
static const char *trueString = "true";
static const char *falseString = "false";

int isdigit(int c)
{
	if (c >= '0' && c <= '9')
		return true;

	return false;
}

bool isalpha(char c) {
	if (c >= 'a' && c <= 'z')
		return true;

	if (c >= 'A' && c <= 'Z')
		return true;

	return false;
}

size_t strlen(const char* str)
{
	size_t i = 0;

	while (*str++)
		i++;

	return i;
}

/// Prints a given number
///
/// @param number The Number to print
/// @param base The base to print the number in
/// @param minLength Min length which wil be padded
/// @param useWhitespacePadding Pad with whitespace instead of zeros
static size_t print_number(file_t f, uint32_t number, uint8_t base, uint32_t minLength, bool useWhitespacePadding)
{
	// We only support base 2 to 16
	if (base < 2 || base > 16)
		return 0;

	char _buffer[25];
	char* buffer = &_buffer[25];
	uint32_t usedLength = 0;

	do {
		uint8_t remainder;

		remainder = number % base;
		number = number / base;

		*(--buffer) = numberDefinitions[remainder];

		usedLength++;
	} while(number > 0 && usedLength < 25);

	// Now padd the thing.
	for (; usedLength < minLength; usedLength++) {
		if (useWhitespacePadding)
			*(--buffer) = ' ';
		else
			*(--buffer) = '0';
	}

	return write(f, buffer, usedLength);
}

size_t fprintf(file_t f, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	size_t len = vfprintf(f, format, args);
	va_end(args);

	return len;
}

size_t vfprintf(file_t f, const char* format, va_list args)
{
	assert(format != NULL, "");

	const char* fmt_start;
	size_t len = 0;
	int res = 0;

	while (*format != '\0') {
		// We have a format specifier here
		// so we need to evaluate it.
		if (*format == '%') {
			fmt_start = format;
			format++;

			if (*format == '%') {
				res = write(f, format, 1);

				if (res < 0)
					return len;

				len += res;
			}
			// It is in deed an format
			else {
				bool useWhitespacePadding = true;
				uint32_t minLength = 0;
				int32_t precision = -1;

				// Read the length specifier
				// if there is any.
				while (isdigit(*format)) {
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

					while (isdigit(*format)) {
						precision = precision * 10 + (*format - '0');
						format++;
					}
				}

				if (*format == 'l') {
					format++;
				}

				// Now we've parsed away the formatting options
				// let look at the format
				switch(*format) {
					// Pointer
					case 'p':
						minLength = 8;
						useWhitespacePadding = false;

						res = write(f, "*", 1);
						if (res < 0)
							return len;
						len += res;

					// Hex number
					case 'X':
					case 'x':
					{
						uint32_t val = va_arg(args, uint32_t);
						if (*format == 'x') {
							res = write(f, "0x", 2);
							if (res < 0)
								return len;
							len += res;
						}

						len += print_number(f, val, 16, minLength, useWhitespacePadding);
						break;
					}
					// Signed integer
					case 'd':
					case 'i':
					{
						int32_t val = va_arg(args, int32_t);
						if (val < 0) {
							res = write(f, "-", 1);
							if (res < 0)
								return len;
							len += res;

							val = -val;
						}

						len += print_number(f, (uint32_t)val, 10, minLength, useWhitespacePadding);
						break;
					}
					// Unsinged integer
					case 'u':
					{
						uint32_t val = va_arg(args, uint32_t);
						len += print_number(f, val, 10, minLength, useWhitespacePadding);
						break;
					}
					// Boolean
					case 'B':
					{
						uint32_t val = va_arg(args, uint32_t);
						const char* str = val ? trueString : falseString;

						res = write(f, str, strlen(str));
						if (res < 0)
							return len;
						len += res;

						break;
					}
					// String
					case 's':
					{
						const char* str = va_arg(args, char*) ?: "(null)";
						size_t l = strlen(str);

						res = write(f, str, l);
						if (res < 0)
							return len;
						len += res;
						break;
					}
					default:
						// Don't know the format. so we ignore it.
						res = write(f, fmt_start, format - fmt_start);
						if (res < 0)
							return len;
						len += res;
						break;
				}
			}
		}
		else {
			res = write(f, format, 1);
			if (res < 0)
				return len;
			len += res;
		}

		format++;
	}

	return len;
}

int readline(file_t file, char* buffer, size_t len)
{
	int i;

	for (i = 0; i < len - 1;) {
		char c;

		if (read(file, &c, 1) != 1)
			return -1;

		if (c == '\r' || c == '\n') {
			buffer[i++] = '\0';
			char* str = "\r\n";
			write(file, str, strlen(str));
			break;
		}
		// Backspace
		else if (c == 0x7F) {
			if (i > 0) {
				i--;
				char* str = "\033[1D\033[K";
				write(file, str, strlen(str));
			}
		}
		// Ctrl-C
		else if (c == 0x03) {
			char* str = "\r\n";
			write(file, str, strlen(str));
			return 0;
		}
		// Received control sequence
		else if (c == 0x1B) {
			if (read(file, &c, 1) != 1)
				return -1;

			// Parameter to follow
			// just read them away
			if (c == '[') {
				while (!isalpha(c)) {
					if (read(file, &c, 1) != 1)
						return -1;
				}
			}
		}
		else {
			write(file, &c, 1);
			buffer[i++] = c;
		}
	}

	buffer[i+1] = '\0';

	return i;
}

int memcmp(const void* _s1, const void* _s2, size_t n)
{
	const char* s1 = _s1;
	const char* s2 = _s2;

	for (; n > 0 && *s1 == *s2; --n, --s1, --s2)
		;

	if (n == 0)
		return 0;
	else if (*s1 < *s2)
		return -1;
	else if (*s1 > *s2)
		return 1;

	// All cases handled
	unreachable();
}

int strcmp(const char* s1, const char* s2)
{
	return strncmp(s1, s2, SIZE_MAX);
}

int strncmp(const char* s1, const char* s2, size_t n)
{
	for (; n > 0 && *s1 && *s2 && *s1 == *s2; --n, ++s1, ++s2)
		;

	if (n == 0)
		return 0;
	else if (*s1 < *s2)
		return -1;
	else if (*s1 > *s2)
		return 1;
	else // *s1 may be equal to *s2 here, wenn both are \0 but n is still > 0
		return 0;
}

void* memcpy(void* restrict _dst, const void* restrict _src, size_t n)
{
	char* restrict dst = _dst;
	const char* restrict src = _src;

	for (; n > 0; --n, ++dst, ++src)
		*dst = *src;

	return _dst;
}

void* memmove(void* _dst, const void* _src, size_t n)
{
	char* dst = _dst;
	const char* src = _src;

	if (dst < src) { // Forward
		for (; n > 0; --n, ++src, ++dst)
			*dst = *src;
	}
	else if (dst > src) { // Backward
		src += n;
		dst += n;

		for (; n > 0; --n, --src, --dst)
			*dst = *src;
	}

	return dst;
}

void* memset(void* _b, char c, size_t n)
{
	char* b = _b;

	for (; n > 0; --n, ++b)
		*b = c;

	return b;
}

const char* strchr(const char *s, int c)
{
	for (; *s != '\0'; ++s)
		if (*s == c)
			return s;

	return NULL;
}

char *strsep(char **stringp, const char *delim)
{
	assert(stringp, "stringp can't be NULL");
	assert(delim, "delim can't be NULL");

	char* found = *stringp;
	char* remaining = found;

	if (!stringp)
		return NULL;

	for (; *remaining != '\0'; ++remaining) {
		if (strchr(delim, *remaining)) {
			*remaining = '\0';
			++remaining;
			break;
		}
	}

	if (*remaining == '\0')
		*stringp = NULL;
	else
		*stringp = remaining;

	return found;
}

char* strsep_ext(char** stringp, const char* delim) {
	char* value;

	do {
		value = strsep(stringp, delim);
	} while (value && strlen(value) == 0);

	return value;
}

uint32_t atoi(const char* c)
{
	uint32_t i = 0;

	for (; *c != '\0' && isdigit(*c); c++) {
		i = 10 * i + ((*c)-'0');
	}

	return i;
}
