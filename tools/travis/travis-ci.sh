#!/bin/bash

# Find base dir
DIR="$( cd "$( dirname "$(type -p $0)" )" && pwd )"

eval $($DIR/../env.sh -p)

export PATH=Toolchain/ninja-build:$PATH

./test.sh || exit 1
