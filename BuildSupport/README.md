TheOS BuildSupport
==================

This folder contains files that help building TheOS:
 * a description to build a toolchain
 * helper scripts for building TheOS

TheOS building will base upon rake.

Object files will reside in an obj dir let the rest be clean.

Currently we use the system clang of Mac OS X and therefor
the only build platform tested is Mac OS X ;)

Build the toolchain
-------------------

TOOLCHAIN_ROOT must point to the fullpath of BuildSupport/toolchain

I use bintuils 2.22 and gdb 7.5 on osx 10.8

Download the last release of binutils and compile it

 $ ./configure --prefix="$TOOLCHAIN_ROOT" --target=i386-unkown-elf --program-prefix=i386-theos-elf-
 $ make
 $ make install

Download the las release of gdb and compile it

 $ ./configure --prefix="$TOOLCHAIN_ROOT" --target=i386-unkown-elf --program-prefix=i386-theos-elf-
 $ make
 $ make install

Building clang
--------------

Checkout llvm and clang as described in http://clang.llvm.org/get_started.html

Patch llvm and clang.

 $ mkdir llvm-build
 $ cd llvm-build
 $ ../llvm/configure --target=i386-unkown-theos --prefix="$TOOLCHAIN_ROOT"
 $ make
 $ make install