#!/bin/bash

# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

. $BASEDIR/env.sh

./configure.rb -b $FIRMWARE_BOARD || exit 1
Toolchain/ninja-build/ninja || exit 1
