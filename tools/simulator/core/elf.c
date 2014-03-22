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

#include "elf.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#if 0
#define elf_debug(...) printf("[ELF] "__VA_ARGS__)
#else
#define elf_debug(...)
#endif

enum {
	ELF_IDENT_MAGIC0  = 0,
	ELF_IDENT_MAGIC1  = 1,
	ELF_IDENT_MAGIC2  = 2,
	ELF_IDENT_MAGIC3  = 3,

	ELF_IDENT_CLASS   = 4,
	ELF_IDENT_DATA    = 5,
	ELF_IDENT_VERSION = 6,
	ELF_IDENT_PAD     = 7,

	ELF_IDENT_N       = 16,
};

const uint8_t ELF_MAGIC0 = 0x7F;
const uint8_t ELF_MAGIC1 = 'E';
const uint8_t ELF_MAGIC2 = 'L';
const uint8_t ELF_MAGIC3 = 'F';

enum {
	ELF_CLASS_NONE = 0,
	ELF_CLASS_32   = 1,
	ELF_CLASS_64   = 2,
};

enum {
	ELF_DATA_NONE = 0,
	ELF_DATA_LSB  = 1,
	ELF_DATA_MSB  = 2,
};

struct elf_header {
	uint8_t ident[ELF_IDENT_N];
	uint16_t type;
	uint16_t machine;
	uint32_t version;

	uint32_t entry;

	uint32_t phoff;
	uint32_t shoff;

	uint32_t flags;

	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;

	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndex;
};

enum {
	ELF_PROG_TYPE_NULL = 0,
	ELF_PROG_TYPE_LOAD = 1,
};

struct elf_prog_header {
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesz;
	uint32_t memz;
	uint32_t flags;
	uint32_t align;
};

bool elf_load(mcu_t mcu, const char* file)
{
	int fd = open(file, O_RDONLY);
	bool success;

	if (fd < 0) {
		perror("open");
		return false;
	}

	success = elf_load_fd(mcu, fd);

	close(fd);

	return success;
}

bool elf_load_fd(mcu_t mcu, int fd)
{
	struct elf_header elf_header;

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		return false;
	}

	if (read(fd, &elf_header, sizeof(elf_header)) < 0) {
		perror("read");
		return false;
	}

	if (elf_header.ident[ELF_IDENT_MAGIC0] != ELF_MAGIC0 ||
		elf_header.ident[ELF_IDENT_MAGIC1] != ELF_MAGIC1 ||
		elf_header.ident[ELF_IDENT_MAGIC2] != ELF_MAGIC2 ||
		elf_header.ident[ELF_IDENT_MAGIC3] != ELF_MAGIC3 ) {
		printf("Not an elf file!\n");
		return false;
	}

	mcu_unlock(mcu);

	for (uint16_t i = 0; i < elf_header.phnum; i++) {
		struct elf_prog_header ph;

		if (lseek(fd, elf_header.phoff + i * elf_header.phentsize, SEEK_SET) < 0) {
			perror("lseek");
			mcu_lock(mcu);
			return false;
		}

		if (read(fd, &ph, sizeof(ph)) < 0) {
			perror("read");
			mcu_lock(mcu);
			return false;
		}

		if (ph.type == ELF_PROG_TYPE_LOAD) {
			elf_debug("[ELF] LOAD %x:%x to %x:%x\n", ph.offset, ph.filesz, ph.paddr, ph.memz);

			if (lseek(fd, ph.offset, SEEK_SET) < 0) {
				perror("lseek");
				mcu_lock(mcu);
				return false;
			}

			for (uint32_t j = 0; j < ph.filesz; j++) {
				uint8_t byte;

				if (read(fd, &byte, 1) < 0) {
					perror("read");
					mcu_lock(mcu);
					return false;
				}

				if (!mcu_util_write8(mcu, ph.paddr + j, byte)) {
					mcu_lock(mcu);
					return false;
				}
			}
		}
	}

	mcu_lock(mcu);

	return true;
}
