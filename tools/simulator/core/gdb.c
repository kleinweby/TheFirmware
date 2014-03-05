//
// Copyright (c) 2014, Christian Speich
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

#include "gdb.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

struct gdb {
	mcu_t mcu;

	ev_io socket_io;

	int gdb_fd;

	// Receiving
	char* rev_buffer;
	size_t rev_buffer_filled;
	size_t rev_buffer_length;
	ev_io recv_io;

	// Sending
	char packet_checksum;
	ev_io send_io;

	struct mcu_callbacks mcu_callbacks;

	//
	struct ev_loop* loop;
};

#if 0
#define gdb_debug(...) printf(__VA_ARGS__)
#else
#define gdb_debug(...)
#endif

#define FIXUP_GDB(a, b) ((gdb_t)((uintptr_t)a - __builtin_offsetof(struct gdb, b)))

static const int GDB_BUF = 255;

static void gdb_mcu_did_halt(mcu_t mcu, halt_reason_t reason, void* context);
static void gdb_accept_callback(struct ev_loop *loop, ev_io *w, int revents);
static void gdb_read_callback(struct ev_loop *loop, ev_io *w, int revents);
static void gdb_write_callback(struct ev_loop *loop, ev_io *w, int revents);

gdb_t gdb_create(struct ev_loop* loop, int port, mcu_t mcu)
{
	static int on = 1;

	struct sockaddr_in addr;
	gdb_t gdb = calloc(1, sizeof(struct gdb));

	if (gdb == NULL) {
		perror("calloc");
		return NULL;
	}

	gdb->mcu = mcu;
	gdb->loop = loop;

	{
		int fd  = socket(AF_INET, SOCK_STREAM, 0);

		if (fd < 0) {
			perror("socket");
			return NULL;
		}

		ev_io_init(&gdb->socket_io, gdb_accept_callback, fd, EV_READ);
	}

	// if (ioctl(gdb->socket_fd, FIONBIO, (char *)&on) < 0) {
	// 	perror("ioctl");
	// 	return NULL;
	// }

	bzero((char *) &addr, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
 	addr.sin_port        = htons(port);

 	if (bind(gdb->socket_io.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
 		perror("bind");
 		return NULL;
 	}


	if (setsockopt(gdb->socket_io.fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0) {
    	perror("setsockopt");
    	return NULL;
    }

    if (listen(gdb->socket_io.fd, 5) < 0) {
    	perror("listen");
    	return NULL;
    }

   	gdb->gdb_fd = -1;
   	gdb->rev_buffer_length = 4 * 1024;
   	gdb->rev_buffer_filled = 0;
   	gdb->rev_buffer = malloc(gdb->rev_buffer_length);

   	if (!gdb->rev_buffer) {
   		perror("malloc");
   		return NULL;
   	}

   	gdb->mcu_callbacks.mcu_did_halt = gdb_mcu_did_halt;
   	gdb->mcu_callbacks.context = gdb;

   	mcu_add_callbacks(mcu, &gdb->mcu_callbacks);

   	ev_io_init(&gdb->recv_io, gdb_read_callback, gdb->gdb_fd, EV_READ);
   	ev_io_init(&gdb->send_io, gdb_write_callback, gdb->gdb_fd, EV_WRITE);

   	ev_io_start(gdb->loop, &gdb->socket_io);

	return gdb;
}

static void gdb_client_close(gdb_t gdb)
{
	printf("gdb disconnected\n");
	close(gdb->gdb_fd);
	gdb->gdb_fd = -1;
	ev_io_stop(gdb->loop, &gdb->recv_io);
	ev_io_stop(gdb->loop, &gdb->send_io);
	gdb->rev_buffer_filled = 0;
}

static void gdb_client_error(gdb_t gdb)
{
	perror("gdb-client");
	gdb_client_close(gdb);
}

static bool gdb_send_ack(gdb_t gdb) {
	static const char* ack = "+\n";

	gdb_debug("gdb-send: %s", ack);
	if (write(gdb->gdb_fd, ack, strlen(ack)) < 0) {
		gdb_client_error(gdb);
		return false;
	}

	return true;
}

static bool gdb_send_nack(gdb_t gdb) {
	static const char* ack = "-\n";

	gdb_debug("gdb-send: %s", ack);
	if (write(gdb->gdb_fd, ack, strlen(ack)) < 0) {
		gdb_client_error(gdb);
		return false;
	}

	return true;
}

static bool gdb_send_packet_begin(gdb_t gdb) {
	gdb->packet_checksum = 0;

	gdb_debug("[gdb] > $");

	if (write(gdb->gdb_fd, "$", 1) < 1) {
		gdb_client_error(gdb);
		return false;
	}

	return true;
}

static bool gdb_send_packet_char(gdb_t gdb, char c) {

	if (c == '$' || c == '#' || c == '}') {
		gdb->packet_checksum += '}';

		gdb_debug("}");
		if (write(gdb->gdb_fd, "}", 1) < 1) {
			gdb_client_error(gdb);
			return false;
		}

		c ^= 0x20;
	}

	gdb_debug("%c", c);
	gdb->packet_checksum += c;
	if (write(gdb->gdb_fd, &c, 1) < 1) {
		gdb_client_error(gdb);
		return false;
	}

	return true;
}

static bool gdb_send_packet_str(gdb_t gdb, const char* str) {
	while (*str)
		if (!gdb_send_packet_char(gdb, *str++))
			return false;

	return true;
}

#define TO_HEX(i) ((i) <= 9 ? '0' + (i) : 'a' - 10 + (i))

static bool gdb_send_packet_hex(gdb_t gdb, uint32_t number, int length) {
	for (int i = 0; i < length; i++) {
		char c = (number >> (i * 8)) & 0xFF;
		char str[] = {
			TO_HEX((c >> 4) & 0xF),
			TO_HEX((c >> 0) & 0xF),
			'\0'
		};

		if (!gdb_send_packet_str(gdb, str))
			return false;
	}

	return true;
}

static bool gdb_send_packet_end(gdb_t gdb) {

	gdb_debug("#");
	if (write(gdb->gdb_fd, "#", 1) < 1) {
		gdb_client_error(gdb);
		return false;
	}

	if (!gdb_send_packet_hex(gdb, gdb->packet_checksum, 1))
		return false;

	gdb_debug("\n");

	return true;
}

static bool gdb_mcu_write_byte(mcu_t mcu, uint32_t addr, char c)
{
	uint16_t val;

	if (!mcu_fetch16(mcu, addr & ~1, &val))
		return false;

	if (addr & 1)
		val = (val & 0x00FF) | (c << 8);
	else
		val = (val & 0xFF00) | c;

	if (!mcu_write16(mcu, addr & ~1, val))
		return false;

	return true;
}


static bool gdb_mcu_fetch_byte(mcu_t mcu, uint32_t addr, char* c)
{
	uint16_t val;

	if (!mcu_fetch16(mcu, addr & ~1, &val))
		return false;

	if (addr & 1)
		*c = (val & 0xFF00) >> 8;
	else
		*c = (val & 0x00FF);

	return true;
}

static bool gdb_handle_packet(gdb_t gdb, char* packet, size_t len)
{
	packet++;
	len--;

	{
		char checksum = 0;
		char* buf;

		for (buf = packet; *buf != '#' && buf < packet + len; buf++)
			checksum += *buf;

		if (buf >= packet + len) {
			printf("Packet does not contain checksum!\n");
			gdb_send_nack(gdb);
			return false;
		}

		// mark end of message here and setp over it
		*buf++ = '\0';

		char checksum_buf[] = { buf[0], buf[1], '\0' };

		char recv_checksum = strtol(checksum_buf, NULL, 16);

		if (recv_checksum != checksum) {
			printf("Packet checksum %02x does not match %02x\n", checksum, recv_checksum);
			gdb_send_nack(gdb);
			return false;
		}
	}

	gdb_send_ack(gdb);

	// Decode gdb packet
	{
		size_t len = strlen(packet);
		size_t i = 0;

		for (char* buf = packet; *buf != '\0'; buf++, i++) {
			if (*buf == '}') {
				memmove(buf, buf+1, len - i - 1);
				*buf ^= 0x20;
			}
		}
	}

	switch(*packet++) {
		case 'q':
			gdb_send_packet_begin(gdb);
			if (strncmp(packet, "Supported", strlen("Supported")) == 0) {
				gdb_send_packet_str(gdb, "PacketSize=119");
			}
			else if (strncmp(packet, "C", strlen("C")) == 0) {
				gdb_send_packet_str(gdb, "");
			}
			else if (strncmp(packet, "Offsets", strlen("Offsets")) == 0) {
				gdb_send_packet_str(gdb, "Text=0;Data=0;Bss=0");
			}
			gdb_send_packet_end(gdb);
			break;
		case '?':
			gdb_send_packet_begin(gdb);
			gdb_send_packet_str(gdb, "S");
			gdb_send_packet_hex(gdb, mcu_halt_reason(gdb->mcu), 1);
			gdb_send_packet_end(gdb);
			break;
		case 'H':
			gdb_send_packet_begin(gdb);
			gdb_send_packet_str(gdb, "OK");
			gdb_send_packet_end(gdb);
			break;
		case 'g':
			gdb_send_packet_begin(gdb);
			for (reg_t reg = 0; reg < reg_gdb_count; reg++)
				gdb_send_packet_hex(gdb, mcu_read_reg(gdb->mcu, reg), 4);
			gdb_send_packet_end(gdb);
			break;
		case 'p':
		{
			reg_t reg = strtol(packet, NULL, 16);

			gdb_send_packet_begin(gdb);
			if (reg < reg_count)
				gdb_send_packet_hex(gdb, mcu_read_reg(gdb->mcu, reg), 4);
			gdb_send_packet_end(gdb);
			break;
		}
		case 'P':
		{
			reg_t reg = strtol(packet, &packet, 16);

			packet++; // =
			uint32_t val = strtol(packet, NULL, 16);

			// When gdb sets the pc to 0x0, it actually means
			// it want's to reset the mcu
			if (reg == REG_PC && val == 0x0)
				mcu_reset(gdb->mcu);
			else
				mcu_write_reg(gdb->mcu, reg, val);

			gdb_send_packet_begin(gdb);
			gdb_send_packet_str(gdb, "OK");
			gdb_send_packet_end(gdb);
			break;
		}
		case 'm':
		{
			uint32_t addr = strtol(packet, &packet, 16);
			packet++;
			uint32_t length = strtol(packet, NULL, 16);

			gdb_send_packet_begin(gdb);

			for (; length > 0; length--, addr++) {
				char c;

				if (!gdb_mcu_fetch_byte(gdb->mcu, addr, &c)) {
					break;
				}

				gdb_send_packet_hex(gdb, c, 1);
			}

			gdb_send_packet_end(gdb);
			break;
		}

		case 'v':
			gdb_send_packet_begin(gdb);
			gdb_send_packet_end(gdb);
			break;

		case 's':
			// One step
			mcu_step(gdb->mcu);

			gdb_send_packet_begin(gdb);
			gdb_send_packet_str(gdb, "S05");
			gdb_send_packet_end(gdb);
			break;

		case 'c':
			mcu_resume(gdb->mcu);

			break;

		case 'X':
		{
			uint32_t addr = strtol(packet, &packet, 16);
			packet++;
			uint32_t length = strtol(packet, &packet, 16);
			packet++; // After the :
			bool sucess = true;

			mcu_unlock(gdb->mcu);

			for (; length > 0; length--, packet++, addr++) {
				if (!gdb_mcu_write_byte(gdb->mcu, addr, *packet)) {
					sucess = false;
					break;
				}
			}

			mcu_lock(gdb->mcu);

			gdb_send_packet_begin(gdb);
			if (sucess)
				gdb_send_packet_str(gdb, "OK");
			else
				gdb_send_packet_str(gdb, "EFF");
			gdb_send_packet_end(gdb);

			break;
		}

		default:
			gdb_send_packet_begin(gdb);
			gdb_send_packet_end(gdb);
	}

	return true;
}

static void gdb_mcu_did_halt(mcu_t mcu, halt_reason_t reason, void* context)
{
	gdb_t gdb = (gdb_t)context;

  // Don't tell gdb when the mcu only entered a sleep state
	if (reason >= 0 && gdb->gdb_fd >= 0) {
		gdb_send_packet_begin(gdb);
		gdb_send_packet_str(gdb, "S");
		gdb_send_packet_hex(gdb, reason, 1);
		gdb_send_packet_end(gdb);
	}
}

static void gdb_accept_callback(struct ev_loop *loop, ev_io *w, int revents)
{
	gdb_t gdb = FIXUP_GDB(w, socket_io);

	if (revents & EV_READ) {
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);

		int fd = accept(gdb->socket_io.fd, (struct sockaddr *)&addr, &addr_len);

		if (fd < 0) {
			perror("accept");
			return;
		}

		// Reject connection
		if (gdb->gdb_fd >= 0) {
			close(fd);
		}
		else {
			gdb->gdb_fd = fd;

			ev_io_set(&gdb->recv_io, fd, EV_READ);
			ev_io_set(&gdb->send_io, fd, EV_WRITE);

			ev_io_start(gdb->loop, &gdb->recv_io);

			printf("Connected to gdb client\n");
		}
	}
}

static void gdb_read_callback(struct ev_loop *loop, ev_io *w, int revents)
{
	gdb_t gdb = FIXUP_GDB(w, recv_io);

	if (revents & EV_READ) {
		ssize_t len;
		bool hasBreak = false;

		len = read(gdb->gdb_fd, gdb->rev_buffer + gdb->rev_buffer_filled, gdb->rev_buffer_length - gdb->rev_buffer_filled - 1);
		if (len == 0) {
			gdb_client_close(gdb);
		}
		else if (len < 0) {
			if (errno == EAGAIN)
				return;

			gdb_client_error(gdb);
			return;
		}

		gdb->rev_buffer_filled += len;
		gdb->rev_buffer[gdb->rev_buffer_filled] = '\0';

		// Try to find a packet start
		for (char* b = gdb->rev_buffer; b < gdb->rev_buffer + gdb->rev_buffer_filled; b++) {
			if (*b == 0x03)
				hasBreak = true;
			else if (*b == '$') { // Packet start
				// We now move the beginning of the packet to the beginning of the buffer
				gdb->rev_buffer_filled -= b - gdb->rev_buffer;
				memmove(gdb->rev_buffer, b, gdb->rev_buffer_filled);
				break;
			}
		}

		// We have a packet
		if (*gdb->rev_buffer == '$') {
			char* end = gdb->rev_buffer;

			for (; *end != '#' && end < gdb->rev_buffer + gdb->rev_buffer_filled; end++)
				;

			// The packet is not fully in the buffer
			if (*end != '#')
				return;

			// '#' + two checksum bytes
			end += 3;

			// end is no longer valid
			if (end > gdb->rev_buffer + gdb->rev_buffer_filled)
				return;

			gdb_debug("[gdb] < ");
			for (char* b = gdb->rev_buffer; b < end; b++) {
				unsigned char c = *b;
				if (isprint(c))
					gdb_debug("%c", c);
				else
					gdb_debug("\\0x%02X", c);
			}
			gdb_debug("\n");

			gdb_handle_packet(gdb, gdb->rev_buffer, end - gdb->rev_buffer);

			// remove the handled packet
			gdb->rev_buffer_filled -= end - gdb->rev_buffer;
			memmove(gdb->rev_buffer, end, gdb->rev_buffer_filled);

			for (char *b = gdb->rev_buffer ; *b != '$' && b < gdb->rev_buffer + gdb->rev_buffer_filled; b++)
				if (*b == 0x03)
					hasBreak = true;
		}

		if (hasBreak)
			mcu_halt(gdb->mcu, HAL_TRAP);
	}
}

static void gdb_write_callback(struct ev_loop *loop, ev_io *w, int revents)
{
	gdb_t gdb = FIXUP_GDB(w, send_io);

	#pragma unused(gdb)
}
