#!/bin/bash

# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

./configure.rb -b $FIRMWARE_BOARD
ninja
