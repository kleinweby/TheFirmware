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
/// @file semaphore.h
/// @defgroup semaphore
/// @{

#pragma once

#include <stdint.h>
#include <list.h>
#include <runtime.h>

typedef struct semaphore* semaphore_t;

/// @internal
struct semaphore {
  /// The current value of the semaphore
  int8_t value;
  /// A list of waiting threads. Should be woken in the order they arrived.
  list_t queue;
};

/// Creates a new heap allocated sempahore
///
/// @param initial_value Initial value that the semaphore holds
///
/// @returns a newly allocated semaphore
/// @retval NULL if allocation failed
semaphore_t semaphore_create(int8_t initial_value);

/// Initializes a semaphore at a given location
void semaphore_init(semaphore_t semaphore, int8_t initial_value);

/// Destroys a heap allocated sempahore
void semaphore_destory(semaphore_t semaphore);

/// Clean up a semaphore at a given location
void semaphore_cleanup(semaphore_t semaphore);

/// Signal a given semaphore
void semaphore_signal(semaphore_t semaphore);

/// Wait on a semaphore if needed indefinitly
void semaphore_wait(semaphore_t semaphore);

/// @}
