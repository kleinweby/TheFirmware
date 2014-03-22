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

#include "malloc.h"

#include "runtime.h"

struct malloc_header {
	size_t size;
};

struct free_header {
	size_t size;
	struct free_header* next;
};

static struct free_header* free_head;

void* malloc(size_t size)
{
	struct malloc_header* ptr = malloc_raw(sizeof(struct malloc_header) + size);

	ptr->size = size;

	return (void*)((int)ptr + sizeof(struct malloc_header));
}

void free(void* _ptr)
{
	struct malloc_header* ptr = (struct malloc_header*)((int)_ptr - sizeof(struct malloc_header));

	free_raw(ptr, ptr->size + sizeof(struct malloc_header));
}

void* malloc_raw(size_t size)
{
	assert(size >= sizeof(struct free_header), "malloc_raw size must be >= sizeof(struct free_header)");

	if (free_head->size >= size) {
		struct free_header* ptr = free_head;

		free_head = free_head->next;

		if (ptr->size - size >= sizeof(struct free_header)) {
			free_raw((void*)((int)ptr + size), ptr->size - size);
			ptr->size -= size;
		}

		return ptr;
	}

	for (struct free_header* prev = free_head; prev->next != NULL; prev = prev->next) {
		struct free_header* ptr = prev->next;

		if (ptr->size >= size) {
			prev->next = ptr->next;

			if (ptr->size - size >= sizeof(struct free_header)) {
				free_raw((void*)((int)ptr + size), ptr->size - size);
				ptr->size -= size;
			}

			return ptr;
		}
	}

	return NULL;
}

void free_raw(void* _ptr, size_t size)
{
	assert(_ptr != NULL, "cannot free NULL");
	assert(size >= sizeof(struct free_header), "free_raw size must be >= sizeof(struct free_header)");

	struct free_header* ptr = _ptr;

	ptr->size = size;
	ptr->next = NULL;

	if (free_head == NULL) {
		free_head = ptr;
		return;
	}
	// Free block is before current head
	else if (free_head > ptr) {
		if ((int)free_head + size == (int)ptr) {
			ptr->size += free_head->size;
			ptr->next = free_head->next;
		}
		else {
			ptr->next = free_head;
		}

		free_head = ptr;
	}

	for (struct free_header* block = free_head; block != NULL; block = block->next) {
		if (block < ptr || block->next == NULL) {
			if ((int)block + block->size == (int)ptr) {
				block->size += ptr->size;
				ptr = block;
			}
			else {
				ptr->next = block->next;
				block->next = ptr;
			}

			if (ptr->next && (int)ptr + ptr->size == (int)ptr->next) {
				ptr->size += ptr->next->size;
				ptr->next = ptr->next->next;
			}

			return;
		}
	}
}

size_t get_free_size()
{
	size_t size = 0;

	for (struct free_header* ptr = free_head; ptr != NULL; ptr = ptr->next) {
		size += ptr->size;
	}

	return size;
}

void malloc_init(void* start, void* end)
{
	free_raw(start, end - start);
}
