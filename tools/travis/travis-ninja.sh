#!/bin/bash

# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

pushd $BASEDIR/../../Toolchain > /dev/null
git clone https://github.com/martine/ninja.git ninja-build || exit 1

pushd ninja-build > /dev/null

./bootstrap.py || exit 1

popd > /dev/null
popd > /dev/null
