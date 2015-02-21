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

int readline(file_t file, char* buffer, size_t len)
{
	size_t i;

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
		else if (c == 0x7F | c == 0x8) {
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

void* __aeabi_memset(void* _b, size_t n, char c)
{
	return memset(_b, c, n);
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
	return strtol(c, NULL, 10);
}

uint32_t strtol(const char* c, char **endptr, uint8_t base)
{
	// Autodetect base
	if (base == 0) {
		if (*c == '0' && (*(c+1) == 'x' || *(c+1) == 'X')) {
			base = 16;
			c += 2;
		}
		else if (*c == '0' && (*(c+1) == 'b' || *(c+1) == 'B')) {
			base = 2;
			c += 2;
		}
		else {
			base = 10;
		}
	}

	uint32_t n = 0;
	for (; *c != '\0'; c++) {
		uint8_t i;

		if ('0' <= *c && *c <= '9') {
			i = *c - '0'; 
		}
		else if ('A' <= *c && *c <= 'F') {
			i = *c - 'A' + 10;
		}
		else if ('a' <= *c && *c <= 'f') {
			i = *c - 'a' + 10;
		}
		else {
			break;
		}

		if (i > base) {
			break;
		}

		n *= base;
		n += i;
	}

	if (endptr) {
		*endptr = (char*)c;
	}

	return n;
}
