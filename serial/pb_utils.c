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

#include <pb_utils.h>

#define CRC32MASKREV 0xEDB88320

bool _pb_crc32_ostream_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
	uint32_t crc32 = (uint32_t)stream->state;

	for (size_t i = 0; i < count; i++) {
		for (size_t j = 0; j < 8; j++) {
		if ((crc32 & 1) != ((buf[i] >> j) & 0x1))
             crc32 = (crc32 >> 1) ^ CRC32MASKREV;
        else
             crc32 >>= 1;
     	}
	}

	stream->state = (void*)crc32;

	return true;
}

bool pb_ostream_file_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
	return write((file_t)stream->state, buf, count) == count;
}

static bool pb_static_string_field_encode(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
	const char *str = (const char*)*arg;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

void pb_utils_encode_static_string(pb_callback_t* callback, const char* str)
{
	callback->funcs.encode = pb_static_string_field_encode;
	callback->arg = (void*)str;
}

typedef struct pb_file {
	struct file file;
	pb_ostream_t* stream;
}* pb_file_t;

static int pb_file_write(file_t file, const void* buf, size_t nbytes)
{
	pb_file_t f = (pb_file_t)file;

	pb_write(f->stream, buf, nbytes);

	return nbytes;
}

const struct file_operations pb_file_ops = {
	.write = pb_file_write
};

bool _pb_utils_encode_vastring_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
	struct _pb_utils_vastring_args* args = (struct _pb_utils_vastring_args*)*arg;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    va_list c;
    va_copy(c, args->args);
	size_t size = vfprintf(NULL, args->message, c);
	va_end(c);

	if (!pb_encode_varint(stream, (uint64_t)size))
        return false;

	struct pb_file f = {
		.file.ops = &pb_file_ops,
		.stream = stream
	};

    va_copy(c, args->args);
	vfprintf(&f.file, args->message, args->args);
	va_end(c);

   	return true;
}
