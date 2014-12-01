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

#include <pb.h>
#include <pb_encode.h>

bool _pb_crc32_ostream_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count);

#define pb_crc32_ostream_init {&_pb_crc32_ostream_callback, (void*)0xffffffff, SIZE_MAX, 0}

static inline uint32_t pb_crc32_ostream_get(pb_ostream_t *stream) {
	return (uint32_t)stream->state ^ 0xffffffff;
}

bool pb_ostream_file_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count);
#define pb_ostream_from_file(file) {&pb_ostream_file_callback, (void*)(file), SIZE_MAX, 0}

void pb_utils_encode_static_string(pb_callback_t* callback, const char* str);

struct _pb_utils_vastring_args {
	const char* message;
	va_list args;
};

bool _pb_utils_encode_vastring_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

// Need to do that in an macro do avoid allocating the args struct on heap
#define pb_utils_encode_vastring(callback, format, vaargs) \
	(callback)->funcs.encode = _pb_utils_encode_vastring_callback; \
	(callback)->arg = (void*)&(struct _pb_utils_vastring_args){ \
		.message = format, \
		.args = vaargs \
	}; 
