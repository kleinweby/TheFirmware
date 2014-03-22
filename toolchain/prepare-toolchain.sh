#!/bin/bash

PLATFORM="arm"
TARGET_TRIPLET=arm-none-eabi
TARGET_TRIPLET_CLANG=arm-none-eabi

# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

TOOLCHAIN_PATCHES=$BASEDIR/patches
TOOLCHAIN_DIR=$BASEDIR/$PLATFORM.toolchain
TEMP_DIR=
DOWNLOAD_CACHE=$BASEDIR/downloads

LLVM_REVERSION=185323
LLD_REVERSION=185323
CLANG_REVERSION=185323
COMPILER_RT_REVERSION=185323
CURL_OPTIONS="-L -f -# -C -"
TOOLCHAIN_VERSION=$(git rev-list -1 HEAD -- "$BASEDIR")
TOOLCHAIN_URL=
TOOLCHAIN_PRECOMPILED=0
BINUTILS_VERSION=2.24
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

	trap on_exit EXIT
}

function download_cached {
	FILE_NAME=$1
	URL=$2

	if ! [ -f "$DOWNLOAD_CACHE/$FILE_NAME" ]; then
		log "Download $FILE_NAME"
		curl $CURL_OPTIONS -o "$DOWNLOAD_CACHE/$FILE_NAME.part" "$URL" || exit 1
		mv "$DOWNLOAD_CACHE/$FILE_NAME.part" "$DOWNLOAD_CACHE/$FILE_NAME" || exit 1
	fi

	cp "$DOWNLOAD_CACHE/$FILE_NAME" "$TEMP_DIR/$FILE_NAME" || exit 1
}

function svn_co_cached {
	NAME=$1
	URL=$2
	REV=$3

	log "Checkout $NAME"
	if ! [ -d "$DOWNLOAD_CACHE/$NAME" ]; then
		svn co "$URL"@"$REV" "$DOWNLOAD_CACHE/$NAME" > /dev/null || exit 1
	else
		pushd "$DOWNLOAD_CACHE/$NAME"
		svn up -r$REV > /dev/null || exit 1
		popd
	fi

	rm -rf "$TEMP_DIR/$NAME" || exit 1
	cp -r "$DOWNLOAD_CACHE/$NAME" "$TEMP_DIR/$NAME" || exit 1
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
	download_cached "binutils-$BINUTILS_VERSION.tar.gz" "http://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
	log "Unpack binutils"
	mkdir "$TEMP_DIR/binutils"
	tar xf "$TEMP_DIR/binutils-$BINUTILS_VERSION.tar.gz" -C "$TEMP_DIR/binutils" --strip=1 || exit 1
	rm "$TEMP_DIR/binutils-$BINUTILS_VERSION.tar.gz"
}

function download_llvm {
	svn_co_cached llvm "http://llvm.org/svn/llvm-project/llvm/trunk" "$LLVM_REVERSION"
	svn_co_cached llvm/tools/lld "http://llvm.org/svn/llvm-project/lld/trunk" "$LLD_REVERSION"
}

function download_clang {
	svn_co_cached llvm/tools/clang "http://llvm.org/svn/llvm-project/cfe/trunk" "$CLANG_REVERSION"
	# For some reason compiler-rt doen't like when we're only compiling for some targets
	# it seems to be always expecting to build for all targets and then fails, as
	# it did not produce files for e.g. x86 and therefore lipo cant find them.
	# Have to stick with libgcc for now :(
	# svn_co_cached llvm/projects/compiler-rt "http://llvm.org/svn/llvm-project/compiler-rt/trunk" "$COMPILER_RT_REVERSION"
}

function download_gcc_base_toolchain {
	if ! [ -d "$BASEDIR/arm-gcc.toolchain" ]; then
		download_cached "arm-gcc.tar.gz" "$ARM_GCC_BASE"
		log "Unpack"
		mkdir "$BASEDIR/arm-gcc.toolchain"
		tar xf "$TEMP_DIR/arm-gcc.tar.gz" -C "$BASEDIR/arm-gcc.toolchain" --strip=1 || exit 1
		rm "$TEMP_DIR/arm-gcc.tar.gz"
	fi
}

# Until compiler-rt works we need to pull the whole gcc just to build little libgcc
function download_libgcc {
	LIBGCC_VERSION=4.8.2
	GMP_VERSION=5.1.3
	MPC_VERSION=1.0.2
	MPFR_VERSION=3.1.2
	download_cached "gcc-$LIBGCC_VERSION.tar.bz2" "http://ftp.gnu.org/gnu/gcc/gcc-$LIBGCC_VERSION/gcc-$LIBGCC_VERSION.tar.bz2"
	mkdir "$TEMP_DIR/libgcc"
	tar xf "$TEMP_DIR/gcc-$LIBGCC_VERSION.tar.bz2" -C "$TEMP_DIR/libgcc" --strip=1 || exit 1
	rm "$TEMP_DIR/gcc-$LIBGCC_VERSION.tar.bz2"

	download_cached "gmp-$GMP_VERSION.tar.bz2" "http://ftp.gnu.org/gnu/gmp/gmp-$GMP_VERSION.tar.bz2"
	mkdir "$TEMP_DIR/libgcc/gmp"
	tar xf "$TEMP_DIR/gmp-$GMP_VERSION.tar.bz2" -C "$TEMP_DIR/libgcc/gmp" --strip=1 || exit 1
	rm "$TEMP_DIR/gmp-$GMP_VERSION.tar.bz2"

	download_cached "mpc-$MPC_VERSION.tar.gz" "http://ftp.gnu.org/gnu/mpc/mpc-$MPC_VERSION.tar.gz"
	mkdir "$TEMP_DIR/libgcc/mpc"
	tar xf "$TEMP_DIR/mpc-$MPC_VERSION.tar.gz" -C "$TEMP_DIR/libgcc/mpc" --strip=1 || exit 1
	rm "$TEMP_DIR/mpc-$MPC_VERSION.tar.gz"

	download_cached "mpfr-$MPFR_VERSION.tar.xz" "http://ftp.gnu.org/gnu/mpfr/mpfr-$MPFR_VERSION.tar.xz"
	mkdir "$TEMP_DIR/libgcc/mpfr"
	tar xf "$TEMP_DIR/mpfr-$MPFR_VERSION.tar.xz" -C "$TEMP_DIR/libgcc/mpfr" --strip=1 || exit 1
	rm "$TEMP_DIR/mpfr-$MPFR_VERSION.tar.xz"
}

function download_gdb {
	download_cached "gdb-$GDB_VERSION.tar.bz2" "http://ftp.gnu.org/gnu/gdb/gdb-$GDB_VERSION.tar.bz2"
	mkdir "$TEMP_DIR/gdb"
	tar xf "$TEMP_DIR/gdb-$GDB_VERSION.tar.bz2" -C "$TEMP_DIR/gdb" --strip=1 || exit 1
	rm "$TEMP_DIR/gdb-$GDB_VERSION.tar.bz2"
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
	./configure --prefix="$TOOLCHAIN_DIR" --target="$TARGET_TRIPLET" --with-build-sysroot="$TOOLCHAIN_DIR" --without-docs --disable-werror || exit 1
 	make -j$MAKE_JOBS || exit 1
 	make install || exit 1
 	popd
}

function compile_libgcc {
	log "Compile LIBGCC"
	mkdir "$TEMP_DIR/libgcc-build"
	pushd "$TEMP_DIR/libgcc-build"
	../libgcc/configure --prefix="$TOOLCHAIN_DIR" --target="$TARGET_TRIPLET" --with-build-sysroot="$TOOLCHAIN_DIR" --without-doc --disable-werror  || exit 1
	make -j$MAKE_JOBS all-target-libgcc || exit 1
	make install-target-libgcc || exit 1
	popd
}

function compile_llvm_clang {
	log "Compile LLVM/CLANG"
	mkdir "$TEMP_DIR/llvm-build"
	pushd "$TEMP_DIR/llvm-build"
	../llvm/configure --target="$TARGET_TRIPLET_CLANG" --prefix="$TOOLCHAIN_DIR" --with-build-sysroot="$TOOLCHAIN_DIR" --disable-docs --disable-static --enable-optimized --disable-assertions --disable-debug-runtime --disable-expensive-checks --enable-bindings=none --enable-targets=arm || exit 1
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

function remove_unneeded_files {
	rm -rf "$TOOLCHAIN_DIR/share/info"
	rm -rf "$TOOLCHAIN_DIR/share/man"
	find "$TOOLCHAIN_DIR" -name '*.a' -and -not -name 'libgcc.a' | xargs rm
}

log "Prepare toolchain for $PLATFORM"

rm -rf "$TOOLCHAIN_DIR"

detect_host

download_precompiled_toolchain
download_gcc_base_toolchain

if [[ $TOOLCHAIN_PRECOMPILED -eq 1 ]]; then
	log "Found a precompiled version..."
	install_precompiled_toolchain
else
	log "No precompiled version avaiable. Compile the toolchain..."

	export PATH="$TOOLCHAIN_DIR/bin:$PATH"

	mkdir -p "$DOWNLOAD_CACHE"

	download_binutils
	download_gdb
	# download_libgcc
	download_llvm
	download_clang

	# patch_llvm_clang

	compile_binutils
	compile_llvm_clang
	# compile_libgcc
	compile_gdb

	remove_unneeded_files

	upload_toolchain
fi
