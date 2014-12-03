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

#include <connection.h>
#include <malloc.h>
#include <pb_utils.h>
#include <cmd.pb.h>
#include <log.h>
#include <identify.pb.h>

const uint8_t kMagicNumber[2] = {0xCA, 0xFE};

serial_connection_t serial_connection_create(file_t fd)
{
	serial_connection_t con = malloc_raw(sizeof(struct serial_connection));

	if (!con) {
		return NULL;
	}

	con->fd = fd;

	return con;
}

void serial_connection_destory(serial_connection_t con)
{
	free_raw(con, sizeof(struct serial_connection));
}

static void serial_connection_handler(serial_connection_t conn);

void serial_connection_handler_spawn(serial_connection_t conn)
{
	assert(conn->handler_thread == NULL, "Connection already has an handler thread");

  	conn->handler_thread = thread_create("serial_handler", STACK_SIZE_MAIN, NULL);

	if (!conn->handler_thread) {
		return;
	}

  	thread_set_function(conn->handler_thread, serial_connection_handler, 1, conn);

  	// Start the console
  	thread_wakeup(conn->handler_thread);
}

static bool serial_connection_read_magic(pb_istream_t *stream)
{
	for (uint8_t i = 0; i < 2; i++) {
		uint8_t c = 0x0;

		if (!pb_read(stream, &c, 1)) {
			log(LOG_LEVEL_ERROR, "Read error %s", stream->errmsg);
			return false;
		}

		if (c != kMagicNumber[i]) {
			log(LOG_LEVEL_ERROR, "Magic was %x, expected %x", (uint32_t)c, (uint32_t)kMagicNumber[i]);
			return false;
		}
	}

	return true;
}

static void serial_connection_handler(serial_connection_t conn)
{
	pb_istream_t stream = pb_istream_from_file(conn->fd);

	for(;;) {
		if (!serial_connection_read_magic(&stream)) {
			continue;
		}

		uint32_t crc32;

		if (!pb_decode_fixed32(&stream, &crc32)){
			continue;
		}

		TheFirmware_Request request;

		pb_extension_t ext;
		TheFirmware_IdentifyRequest r;

		ext.type = &TheFirmware_IdentifyRequest_identifyRequest;
		ext.dest = &r;
		ext.next = NULL;

		request.extensions = &ext;

		if (!pb_decode_delimited(&stream, TheFirmware_Request_fields, &request)){
			log(LOG_LEVEL_ERROR, "Could not decode request: %s", stream.errmsg);
			continue;
		}

		log(LOG_LEVEL_INFO, "Something");
	}
}

