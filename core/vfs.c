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

#include <vfs.h>
#include <string.h>

static const int ERR_NOT_SUPPORTED = -1;

int read(file_t file, void* buf, size_t nbytes)
{
  if (file->ops->read) {
    return file->ops->read(file, buf, nbytes);
  }

  return ERR_NOT_SUPPORTED;
}

int write(file_t file, const void* buf, size_t nbytes)
{
  if (file->ops->write) {
    return file->ops->write(file, buf, nbytes);
  }

  return ERR_NOT_SUPPORTED;
}

int flush(file_t file)
{
  if (file->ops->flush) {
    return file->ops->flush(file);
  }

  // not an error, if unsupported by the backend
  return 0;
}

file_t vnode_open(vnode_t vnode)
{
  if (vnode->kind != VNODE_KIND_REG)
    return NULL;

  const struct vnode_reg_ops* ops = vnode->ops;

  return ops->open(vnode);
}

vnode_t vnode_lookup(vnode_t vnode, const char* name)
{
  if (vnode->kind != VNODE_KIND_DIR)
    return NULL;

  const struct vnode_dir_ops* ops = vnode->ops;

  return ops->lookup(vnode, name);
}

void vnode_readdir(vnode_t vnode, vnode_readdir_callback_t callback, void* context)
{
  if (vnode->kind != VNODE_KIND_DIR)
    return;

  const struct vnode_dir_ops* ops = vnode->ops;

  ops->readdir(vnode, callback, context);
}

int vnode_call(vnode_t vnode, int argc, const char** argv)
{
  if (vnode->kind != VNODE_KIND_CALLABLE)
    return -1;

  const struct vnode_callable_ops* ops = vnode->ops;

  return ops->call(vnode, argc, argv);
}

static vnode_t vfs_root;

void vfs_set_root(vnode_t vnode)
{
  vfs_root = vnode;
}

vnode_t vfs_lookup(const char* path)
{
  vnode_t node = vfs_root;

  while (node && path) {
    node = vnode_lookup(node, path);

    // Advance to next path component
    path = strchr(path, '/');

    if (!path)
      break;

    path++; // After '/'
  }

  return node;
}

struct vfs_dump_context {
  int ident;
  file_t output;
};

static void vfs_dump_callback(vnode_t parent, const char* name, vnode_t child, void* _context)
{
  struct vfs_dump_context* context = _context;

  for (int i = 0; i < context->ident; ++i)
    write(context->output, "  ", 2);

  fprintf(context->output, "%s \t\t; (vnode = %p, kind=%x)\r\n", name, child, child->kind);

  if (child->kind == VNODE_KIND_DIR) {
    context->ident++;
    vnode_readdir(child, vfs_dump_callback, context);
    context->ident--;
  }
}

void vfs_dump(file_t output)
{
  struct vfs_dump_context context = {
    .ident = 1,
    .output = output,
  };
  fprintf(output, "/ \t\t; (vnode = %p)\r\n", vfs_root);
  vnode_readdir(vfs_root, vfs_dump_callback, &context);
}

vnode_t staticfs_lookup(vnode_t _vnode, const char* name)
{
  struct staticfs_vnode* vnode = (struct staticfs_vnode*)_vnode;
  size_t len;

  const char* deli = strchr(name, '/');

  if (deli)
    len = deli - name;
  else
    len = strlen(name);

  for (struct staticfs_vnode_entry* entry = vnode->entries;
       entry->name != NULL; ++entry) {

    // If length does not match skip
    if (strlen(entry->name) != len) {
      continue;
    }

    // Only compare exactly len
    if (strncmp(entry->name, name, len) == 0) {
      return entry->vnode;
    }
  }

  return NULL;
}

void staticfs_readdir(vnode_t _vnode, vnode_readdir_callback_t callback, void* context)
{
  struct staticfs_vnode* vnode = (struct staticfs_vnode*)_vnode;

  for (struct staticfs_vnode_entry* entry = vnode->entries;
       entry->name != NULL; ++entry) {
    callback(_vnode, entry->name, entry->vnode, context);
  }
}

int staticfs_call(vnode_t _vnode, int argc, const char** argv)
{
  struct staticfs_callable_vnode* vnode = (struct staticfs_callable_vnode*)_vnode;

  return vnode->func(argc, argv);
}
