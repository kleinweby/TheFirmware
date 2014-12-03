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

#include <channel.h>

#include <pb_utils.h>
#include <cmd.pb.h>

bool serial_channel_send(serial_channel_t channel, const pb_extension_type_t* extension_type, void* reply)
{
	TheFirmware_Response r = TheFirmware_Response_init_default;
	pb_extension_t ext;

	ext.type = extension_type;
	ext.dest = reply;
	ext.next = NULL;
	r.extensions = &ext;

	r.channel = channel->id;

	uint32_t crc32;
	size_t size;

	{
		pb_ostream_t stream = pb_crc32_ostream_init;

		if (!pb_encode(&stream, TheFirmware_Response_fields, &r))
			return false;

		crc32 = pb_crc32_ostream_get(&stream);
		size = stream.bytes_written;
	}

	pb_ostream_t stream = pb_ostream_from_file(channel->connection->fd);

	// Send Magic Number
	if (!pb_write(&stream, kMagicNumber, 2))
		return false;

	// First send crc
	if (!pb_encode_fixed32(&stream, &crc32)) 
		return false;

	// Then send length
	if (!pb_encode_varint(&stream, (uint64_t)size))
        return false;

	return pb_encode(&stream, TheFirmware_Response_fields, &r);
}
