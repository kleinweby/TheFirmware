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

#include <test.h>
#include <irq.h>
#include <list.h>

#define assert_list(list, msg,...) do { \
	list_entry_t* entries[] = {__VA_ARGS__}; \
	uint32_t count = sizeof(entries)/sizeof(entries[0]); \
	uint32_t i = 0; \
	list_entry_t* pos; \
\
	list_foreach(pos, (list)) { \
		if (i >= count) {\
			test_fail(msg " (list contains to many elements)"); \
		} \
		if (pos != entries[i]) { \
			test_fail(msg " (element is wrong)"); \
		} \
\
		i++; \
	} \
\
	if (i < count) { \
		test_fail(msg " (list contains to few elements)"); \
	}\
} while(0);

static void test_list_add() {
	list_t list;
	list_entry_t a, b, c;

	list_entry_init(&a);
	list_entry_init(&b);
	list_entry_init(&c);

	list_init(&list);

	assert_list(&list, "New list should be empty");

	list_append(&list, &a);

	assert_list(&list, "List should be {a}", &a);

	list_append(&list, &b);

	assert_list(&list, "List should be {a, b}", &a, &b);

	list_append(&list, &c);

	assert_list(&list, "List should be {a, b, c}", &a, &b, &c);
}

DECLARE_TEST("test list add", TEST_AFTER_ARCH_LATE_INIT, test_list_add);

static void test_list_delete() {
	list_t list;
	list_entry_t a, b, c;

	list_entry_init(&a);
	list_entry_init(&b);
	list_entry_init(&c);

	list_init(&list);
	list_append(&list, &a);
	list_append(&list, &b);
	list_append(&list, &c);

	assert_list(&list, "List should be {a, b, c}", &a, &b, &c);

	list_delete(&list, &b);
	assert_list(&list, "List should be {a, c}", &a, &c);

	list_delete(&list, &c);
	assert_list(&list, "List should be {a}", &a);

	list_delete(&list, &a);
	assert_list(&list, "List should be empty");
}

DECLARE_TEST("test list delete", TEST_AFTER_ARCH_LATE_INIT, test_list_delete);

static void test_list_append_same() {
	list_t list;
	list_entry_t a, b, c;

	list_entry_init(&a);
	list_entry_init(&b);
	list_entry_init(&c);

	list_init(&list);
	assert_list(&list, "New list should be empty");

	list_append(&list, &a);
	assert_list(&list, "List should be {a}", &a);

	list_append(&list, &a);
	assert_list(&list, "List should still be {a}", &a);

	list_append(&list, &b);
	assert_list(&list, "List should be {a, b}", &a, &b);

	list_append(&list, &a);
	assert_list(&list, "List should now be {b, a} (append an existing element)", &b, &a);

	list_append(&list, &c);
	assert_list(&list, "List should be {b, a, c}", &b, &a, &c);

	list_append(&list, &a);
	assert_list(&list, "List should now be {b, c, a} (append an existing element)", &b, &c, &a);
}

DECLARE_TEST("test list append same", TEST_AFTER_ARCH_LATE_INIT, test_list_append_same);
