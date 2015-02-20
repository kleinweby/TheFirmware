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

// Heavily inspired and copied from https://github.com/travisg/lk/blob/3b20f66fc1340d8e705e09d6a2f8cf3c8aeca27c/top/init.c 

#include <fw/init.h>

LINKER_SYMBOL(fw_init_begin, struct fw_init_hook*);
LINKER_SYMBOL(fw_init_end, struct fw_init_hook*);

static fw_init_level_t last_level = 0;

void fw_init_run(fw_init_level_t level)
{
	fw_init_level_t last_called_level = last_level;
	const struct fw_init_hook *last = NULL;
    for (;;) {
        const struct fw_init_hook *found = NULL;
        bool seen_last = false;
        for (const struct fw_init_hook *ptr = fw_init_begin; ptr != fw_init_end; ptr++) {
            if (ptr == last)
                seen_last = true;

            if (ptr->level > level)
                continue;
            if (ptr->level < last_called_level)
                continue;
            if (found && found->level <= ptr->level)
                continue;

            if (ptr->level > last_level && ptr->level > last_called_level) {
                found = ptr;
                continue;
            }

            if (ptr->level == last_called_level && ptr != last && seen_last) {
                found = ptr;
                break;
            }
        }

        if (!found)
            break;

        found->hook(found->level);
        last_called_level = found->level;
        last = found;
    }

    last_level = level;
}
