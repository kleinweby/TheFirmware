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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <runtime.h>

#ifndef HAVE_VFS
#define VFS_UNAVAIABLE __attribute__((unavailable("requires compiling with printf support")))
#else
#define VFS_UNAVAIABLE
#endif

typedef ENUM(uint8_t, vnode_kind_t) {
  VNODE_KIND_REG,
  VNODE_KIND_DIR,

  // A callable vnode is a stored executable, which can be called
  // This is introduced to allow embedded binaries, without needing
  // some sort of binary loading (into limited ram)
  // The interface is the same as a main function would have
  VNODE_KIND_CALLABLE,
};

struct vnode {
  vnode_kind_t kind;
  const void* ops;
};

#define VNODE_INIT(_kind, _ops) {.kind = _kind, .ops = _ops}

typedef const struct vnode* vnode_t;

vnode_kind_t vnode_get_kind(vnode_t vnode) VFS_UNAVAIABLE;

struct vnode_reg_ops {
  file_t (*open)(vnode_t vnode);
};

file_t vnode_open(vnode_t vnode) VFS_UNAVAIABLE;

typedef void (*vnode_readdir_callback_t)(vnode_t parent, const char* name, vnode_t child, void* context);

struct vnode_dir_ops {
  // As optimization, it is required of the lookup function to treat any
  // '/' in name as '\0'
  vnode_t (*lookup)(vnode_t vnode, const char* name);
  void (*readdir)(vnode_t vnode, vnode_readdir_callback_t callback, void* context);
};

vnode_t vnode_lookup(vnode_t vnode, const char* name) VFS_UNAVAIABLE;
void vnode_readdir(vnode_t vnode, vnode_readdir_callback_t callback, void* context) VFS_UNAVAIABLE;

struct vnode_callable_ops {
  int (*call)(vnode_t vnode, int argc, const char** argv);
};

int vnode_call(vnode_t vnode, int argc, const char** argv) VFS_UNAVAIABLE;

void vfs_set_root(vnode_t vnode) VFS_UNAVAIABLE;
vnode_t vfs_lookup(const char* path) VFS_UNAVAIABLE;

void vfs_dump(file_t output) VFS_UNAVAIABLE;

// StaticFS

void staticfs_init();

struct staticfs_vnode_entry {
  const char* name;
  const vnode_t vnode;
};

struct staticfs_vnode {
  struct vnode vnode;
  struct staticfs_vnode_entry entries[];
};

struct staticfs_callable_vnode {
  struct vnode vnode;
  int (*func)(int argc, const char** argv);
};

vnode_t staticfs_lookup(vnode_t vnode, const char* name) VFS_UNAVAIABLE;
void staticfs_readdir(vnode_t vnode, vnode_readdir_callback_t callback, void* context) VFS_UNAVAIABLE;
int staticfs_call(vnode_t vnode, int argc, const char** argv) VFS_UNAVAIABLE;

static const struct vnode_dir_ops staticfs_dir_ops = {staticfs_lookup, staticfs_readdir};
static const struct vnode_callable_ops staticfs_callable_ops = { staticfs_call};

#define STATICFS_DIR_ENTRY(_name, _vnode) {.name = _name, .vnode = (vnode_t)&_vnode##_staticnode}
#define STATICFS_DIR_ENTRY_CALLABLE(_name, _func) {.name = _name, .vnode = (vnode_t)&(const struct staticfs_callable_vnode) {.vnode = VNODE_INIT(VNODE_KIND_CALLABLE, &staticfs_callable_ops), .func = _func}}
#define STATICFS_DIR_ENTRY_LAST {.name = NULL}

#define STATICFS_DIR(_name, ...) \
  static const struct staticfs_vnode _name##_staticnode = { \
    .vnode = VNODE_INIT(VNODE_KIND_DIR, &staticfs_dir_ops), \
    .entries = {__VA_ARGS__}, \
  }; \
  static vnode_t _name = (vnode_t)&_name##_staticnode
