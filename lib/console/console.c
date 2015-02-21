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

#include <console.h>
#include <runtime.h>
#include <malloc.h>
#include <thread.h>
#include <firmware_config.h>
#include <string.h>
#include <vfs.h>

struct console {
  file_t file;
  thread_t thread;
};

LINKER_SYMBOL(console_cmds_begin, struct console_cmd*);
LINKER_SYMBOL(console_cmds_end, struct console_cmd*);

static void console_main(console_t console);
static console_t global_console;

console_t console_spawn(file_t file)
{
  assert(file != NULL, "Cannot spawn a console on a NULL file");

  console_t console = malloc_raw(sizeof(struct console));

  if (!console) {
    return NULL;
  }

  console->file = file;
  console->thread = thread_create("console", STACK_SIZE_CONSOLE, NULL);

  if (!console->thread) {
    return NULL;
  }

  thread_set_function(console->thread, console_main, 1, console);

  // Start the console
  thread_wakeup(console->thread);

  global_console = console;
  return console;
}

// Modifies buf
static bool console_parse_command(char* buf, int* _argc, char** _argv) {
  int argc = 0;

  while (*buf != '\0') {
    // Remove spaces
    for (; *buf == ' '; ++buf)
      ;

    if (*buf == '\0')
      break;

    char* arg = buf;

    for (; *buf != '\0' && *buf != ' '; ++buf)
      ;

    if (_argv) {
      _argv[argc] = arg;
    }

    argc++;

    if (*buf == '\0') {
      break;
    }
    else {
      if (_argv)
        *buf = '\0';
      ++buf;
    }
  }

  if (_argc)
    *_argc = argc;

  return true;
}

void console_main(struct console* console) {
  while (1) {
	  char buf[60];
		char* str = "> ";
		write(console->file, str, strlen(str));

		if (readline(console->file, buf, 60) < 0) {
      fprintf(console->file, "error reading\r\n");

		}
		else {
      int argc;

      if (!console_parse_command(buf, &argc, NULL)) {
        fprintf(console->file, "error parsing command\r\n");
        continue;
      }

      // No command entered
      if (argc == 0)
        continue;

      char* argv[argc];

      if (!console_parse_command(buf, &argc, argv)) {
        fprintf(console->file, "error parsing command\r\n");
        continue;
      }

      const struct console_cmd* cmd = NULL;

      for (const struct console_cmd* c = console_cmds_begin; c != console_cmds_end; c++) {
        if (strcmp(c->name, argv[0]) == 0) {
          cmd = c;
          break;
        }
      }

      if (!cmd) {
        fprintf(console->file, "command not found\r\n");
        continue;
      }

      int ret = cmd->func(argc, (const char**)argv);

      if (ret != 0) {
        fprintf(console->file, "command exited with %d\r\n", ret);
      }
		}
	}
}

static int console_help_cmd(int argc, const char** argv)
{
  printf("Known commands:\r\n");
  for (const struct console_cmd* c = console_cmds_begin; c != console_cmds_end; c++) {
    printf("  - %s\r\n", c->name);
  }

  return 0;
}

CONSOLE_CMD(help, console_help_cmd);

size_t printf(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  size_t len = vfprintf(global_console->file, format, args);
  va_end(args);
  return len;
}
