#!/bin/bash

PLATFORM="arm"
TARGET_TRIPLET=arm-none-eabi

# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

TOOLCHAIN_PATCHES=$BASEDIR/patches
TOOLCHAIN_DIR=$BASEDIR/$PLATFORM.toolchain
TEMP_DIR=
trap on_exit EXIT

LLVM_REVERSION=185322
CLANG_REVERSION=185323
COMPILER_RT_REVERSION=185852
CURL_OPTIONS="-L -f -#"
TOOLCHAIN_VERSION=$(git rev-list -1 HEAD -- "$BASEDIR")
TOOLCHAIN_URL=
TOOLCHAIN_PRECOMPILED=0
BINUTILS_VERSION=2.23.2
GDB_VERSION=7.6
MAKE_JOBS=1
HOST=
ARM_GCC_TOOLCHAIN_VERSION="4_7-2013q2-20130614"
ARM_GCC_BASE=

function pushd {
	command pushd $* > /dev/null || exit 1
}

function popd {
	command popd $* > /dev/null || exit 1
}

function on_exit {
	rm -rf "$TEMP_DIR"
	log "On exit"
}

function log {
	echo -e "=> \033[00;32m$@\033[00m"
}

function detect_host {
	HOST=$(echo "$(uname)_$(uname -m)" | tr '[A-Z]' '[a-z]')

	if [ -e /proc/cpuinfo ]; then 
		MAKE_JOBS=$(grep -c ^processor /proc/cpuinfo)
		TEMP_DIR=$(mktemp -d --tmpdir theos-toolchain-XXXXXX)
		ARM_GCC_BASE="https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q2-update/+download/gcc-arm-none-eabi-$ARM_GCC_TOOLCHAIN_VERSION-linux.tar.bz2"
	elif [[ $HOST =~ darwin_* ]]; then
		MAKE_JOBS=$(sysctl -n hw.ncpu)
		TEMP_DIR=$(mktemp -d -t theos-toolchain)
		ARM_GCC_BASE="https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q2-update/+download/gcc-arm-none-eabi-$ARM_GCC_TOOLCHAIN_VERSION-mac.tar.bz2"
	else
		echo "Unsupported host $HOST"
		exit 1
	fi

	TOOLCHAIN_URL="https://chrspeich-thefirmware.s3.amazonaws.com/toolchain/$HOST/$PLATFORM-$TOOLCHAIN_VERSION.tar.xz"
}

function download_precompiled_toolchain {
	log "Checking for precompiled toolchain..."
	curl -s --head "$TOOLCHAIN_URL" | head -n 1 | grep "HTTP/1.[01] [23].." > /dev/null
	if [[ $? -eq 0 ]]; then
		TOOLCHAIN_PRECOMPILED=1

		log "Download precompiled toolchain"
		curl $CURL_OPTIONS -o "$TEMP_DIR/toolchain.tar.xz" "$TOOLCHAIN_URL"
	else
		TOOLCHAIN_PRECOMPILED=0
	fi
}

function download_binutils {
	log "Download binutils"
	curl $CURL_OPTIONS -o "$TEMP_DIR/binutils.tar.gz" "http://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz" || exit 1
	log "Unpack binutils"
	mkdir "$TEMP_DIR/binutils"
	tar xf "$TEMP_DIR/binutils.tar.gz" -C "$TEMP_DIR/binutils" --strip=1 || exit 1
	rm "$TEMP_DIR/binutils.tar.gz"
}

function download_llvm {
	log "Download llvm"
	pushd "$TEMP_DIR"
	svn co "http://llvm.org/svn/llvm-project/llvm/trunk"@"$LLVM_REVERSION" llvm > /dev/null || exit 1
	pushd "llvm/tools/"
	svn co http://llvm.org/svn/llvm-project/lld/trunk lld > /dev/null || exit 1
	popd
	popd
}

function download_clang {
	log "Download clang"
	pushd "$TEMP_DIR/llvm"
	pushd "tools"
	svn co "http://llvm.org/svn/llvm-project/cfe/trunk"@"$CLANG_REVERSION" clang > /dev/null || exit 1
	popd
	# pushd "projects"
	# svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk@"$COMPILER_RT_REVERSION" compiler-rt || exit 1
	# popd
	popd
}

function download_gcc_base_toolchain {
	log "Download gcc base toolchain"
	curl $CURL_OPTIONS -o "$TEMP_DIR/arm-gcc.tar.gz" "$ARM_GCC_BASE" || exit 1
	log "Unpack"
	mkdir "$BASEDIR/arm-gcc.toolchain"
	tar xf "$TEMP_DIR/arm-gcc.tar.gz" -C "$BASEDIR/arm-gcc.toolchain" --strip=1 || exit 1
	rm "$TEMP_DIR/arm-gcc.tar.gz"
}

function download_gdb {
	log "Download gdb"
	curl $CURL_OPTIONS -o "$TEMP_DIR/gdb.tar.gz" "http://ftp.gnu.org/gnu/gdb/gdb-$GDB_VERSION.tar.bz2"
	mkdir "$TEMP_DIR/gdb"
	tar xf "$TEMP_DIR/gdb.tar.gz" -C "$TEMP_DIR/gdb" --strip=1 || exit 1
	rm "$TEMP_DIR/gdb.tar.gz"
}

function patch_llvm_clang {
	log "Patch llvm and clang"
	pushd "$TEMP_DIR/llvm"
	patch -p0 < "$TOOLCHAIN_PATCHES/llvm.patch" || exit 1
	pushd "tools/clang"
	patch -p0 < "$TOOLCHAIN_PATCHES/clang.patch" || exit 1
	popd
	popd
}

function compile_binutils {
	pushd "$TEMP_DIR/binutils"
	./configure --prefix="$TOOLCHAIN_DIR" --target="$TARGET_TRIPLET" --with-build-sysroot="$TOOLCHAIN_DIR" --without-doc --disable-werror || exit 1
 	make -j$MAKE_JOBS || exit 1
 	make install || exit 1
 	popd
}

function compile_llvm_clang {
	log "Compile LLVM/CLANG"
	mkdir "$TEMP_DIR/llvm-build"
	pushd "$TEMP_DIR/llvm-build"
	../llvm/configure --target="$TARGET_TRIPLET" --prefix="$TOOLCHAIN_DIR" --with-build-sysroot="$TOOLCHAIN_DIR" --disable-docs --enable-shared --disable-static --enable-optimized --disable-assertions --disable-debug-runtime --disable-expensive-checks --enable-bindings=none --enable-targets=arm || exit 1
 	make -j$MAKE_JOBS || exit 1
	make install || exit 1
	popd
}

function compile_gdb {
	log "Compile GDB"
	pushd "$TEMP_DIR/gdb"
	./configure --target="$TARGET_TRIPLET" --prefix="$TOOLCHAIN_DIR" --with-python --without-doc --disable-werror  || exit 1
	make -j$MAKE_JOBS || exit 1
	make install || exit 1
	popd
}

function upload_toolchain {
	which travis-artifacts > /dev/null

	if [[ $? -eq 0 ]]; then 
		log "Cache toolchain..."

		log "Compress toolchain archive"
		pushd "$TOOLCHAIN_DIR"
		tar c . | xz > "$TEMP_DIR/$PLATFORM-$TOOLCHAIN_VERSION.tar.xz"
		popd

		log "Upload toolchain archive"
		pushd "$TEMP_DIR"
		travis-artifacts upload --target-path "toolchain/$HOST/" --path "$PLATFORM-$TOOLCHAIN_VERSION.tar.xz"
		popd
	fi
}

function install_precompiled_toolchain {
	log "Unpack precompiled toolchain"
	mkdir "$TOOLCHAIN_DIR"
	xz -d -c "$TEMP_DIR/toolchain.tar.xz" | tar xf - -C "$TOOLCHAIN_DIR"
}

log "Prepare toolchain for $PLATFORM"

rm -rf "$TOOLCHAIN_DIR"

detect_host

download_precompiled_toolchain

if [[ $TOOLCHAIN_PRECOMPILED -eq 1 ]]; then
	log "Found a precompiled version..."
	install_precompiled_toolchain
else
	log "No precompiled version avaiable. Compile the toolchain..."
	download_binutils
	download_gdb
	download_llvm
	download_clang
	download_gcc_base_toolchain

	#patch_llvm_clang

	compile_binutils
	compile_llvm_clang
	compile_gdb

	upload_toolchain
fi
