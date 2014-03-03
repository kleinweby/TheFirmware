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
#include <vfs.h>

#ifdef TESTS_SUPPORTED

static int write_calls = 0;
static int read_calls = 0;
static int flush_calls = 0;

static file_t expected_file = NULL;
static void* expected_buf = NULL;
static size_t expected_nbytes = 0;

static int write_op(file_t file, const void* buf, size_t nbytes)
{
  test_assert(file == expected_file, "passed file is not as expected.");
  test_assert(buf == expected_buf, "passed buf is not as expected");
  test_assert(nbytes == expected_nbytes, "passed nbytes i not as expected");

  write_calls++;
  return 0;
}

static int read_op(file_t file, void* buf, size_t nbytes)
{
  test_assert(file == expected_file, "passed file is not as expected.");
  test_assert(buf == expected_buf, "passed buf is not as expected");
  test_assert(nbytes == expected_nbytes, "passed nbytes i not as expected");

  read_calls++;
  return 0;
}

static int flush_op(file_t file)
{
  test_assert(file == expected_file, "passed file is not as expected.");

  flush_calls++;
  return 0;
}

static void set_expected(file_t file, void* buf, size_t nbytes)
{
  expected_file = file;
  expected_buf = buf;
  expected_nbytes = nbytes;
}

static void reset_calls()
{
  read_calls = 0;
  write_calls = 0;
  flush_calls = 0;
}

#define assert_calls(nwrite, nread, nflush) \
  test_assert(write_calls == nwrite, "write calls not as expected"); \
  test_assert(read_calls == nread, "read calls not as expected"); \
  test_assert(flush_calls == nflush, "flush calls not as expected");

static void test_write_op()
{
  struct file_operations ops = {
    .write = write_op,
    .read = NULL,
    .flush = NULL,
  };
  struct file _f = {
    .pos = 0,
    .ops = &ops,
  };
  file_t f = &_f;

  void* buf = (void*)0xBADBEEF;
  size_t nbytes = 10;

  set_expected(f, buf, nbytes);
  reset_calls();

  test_assert(write(f, buf, nbytes) == 0, "write did return non-zeror");
  assert_calls(1, 0, 0);
}

DECLARE_TEST("test vfs write op", TEST_AFTER_ARCH_LATE_INIT, test_write_op);

static void test_read_op()
{
  struct file_operations ops = {
    .write = NULL,
    .read = read_op,
    .flush = NULL,
  };
  struct file _f = {
    .pos = 0,
    .ops = &ops,
  };
  file_t f = &_f;

  void* buf = (void*)0xBADBEEF;
  size_t nbytes = 10;

  set_expected(f, buf, nbytes);
  reset_calls();

  test_assert(read(f, buf, nbytes) == 0, "read did return non-zeror");
  assert_calls(0, 1, 0);
}

DECLARE_TEST("test vfs read op", TEST_AFTER_ARCH_LATE_INIT, test_read_op);

static void test_flush_op()
{
  struct file_operations ops = {
    .write = NULL,
    .read = NULL,
    .flush = flush_op,
  };
  struct file _f = {
    .pos = 0,
    .ops = &ops,
  };
  file_t f = &_f;

  set_expected(f, NULL, 0);
  reset_calls();

  test_assert(flush(f) == 0, "flush did return non-zeror");
  assert_calls(0, 0, 1);
}

DECLARE_TEST("test vfs flush op", TEST_AFTER_ARCH_LATE_INIT, test_flush_op);

static void test_unsupported_ops()
{
  struct file_operations ops = {
    .write = NULL,
    .read = NULL,
    .flush = NULL,
  };
  struct file _f = {
    .pos = 0,
    .ops = &ops,
  };
  file_t f = &_f;

  void* buf = (void*)0xBADBEEF;
  size_t nbytes = 10;

  test_assert(flush(f) == 0, "flush did return != 0");
  test_assert(write(f, buf, nbytes) < 0, "write did return >= 0");
  test_assert(read(f, buf, nbytes) < 0, "flush did return >= 0");
}

DECLARE_TEST("test vfs unsupported ops", TEST_AFTER_ARCH_LATE_INIT, test_unsupported_ops);

#endif // TESTS_SUPPORTED
