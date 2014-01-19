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

enum {
	CURRENT_TEST_OFFSET =  0x0,
	TOTAL_TESTS_OFFSET  =  0x4,
	DESC_OFFSET         =  0x8,
	STATUS_OFFSET       = 0x12,
	STATUS_DESC_OFFSET  = 0x16,

	SIZE                = 0x20,
};

enum {
	STATUS_PREPARING    = 0x55555555,
	STATUS_RUNNING      = 0xDB6DB6DB,

	STATUS_SKIPPED      = 0x24924924,
	STATUS_PASSED       = 0xBEEFBEEF,
	STATUS_FAILED       = 0xDEADBEEF,
};

uint32_t* unittest_base = (uint32_t*)0xEE000000;
#define UNITTEST(offset) *((uint32_t*)((uint32_t)unittest_base + (offset)))

void arch_test_set_test_count(uint32_t count)
{
	UNITTEST(TOTAL_TESTS_OFFSET) = count;
	unreachable();
}

int32_t arch_test_get_test_current()
{
	return UNITTEST(CURRENT_TEST_OFFSET);
}

void arch_test_set_desc(const char* desc)
{
	UNITTEST(DESC_OFFSET) = (uint32_t)desc;
	UNITTEST(STATUS_OFFSET) = STATUS_RUNNING;
}

void arch_test_skip(const char* reason)
{
	UNITTEST(STATUS_DESC_OFFSET) = (uint32_t)reason;
	UNITTEST(STATUS_OFFSET) = STATUS_SKIPPED;
	unreachable();
}

void arch_test_pass(const char* reason)
{
	UNITTEST(STATUS_DESC_OFFSET) = (uint32_t)reason;
	UNITTEST(STATUS_OFFSET) = STATUS_PASSED;
	unreachable();
}

void arch_test_fail(const char* reason)
{
	UNITTEST(STATUS_DESC_OFFSET) = (uint32_t)reason;
	UNITTEST(STATUS_OFFSET) = STATUS_FAILED;
	unreachable();
}
