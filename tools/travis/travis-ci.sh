#!/bin/bash

# Find base dir
DIR="$( cd "$( dirname "$(type -p $0)" )" && pwd )"

eval $($DIR/../env.sh -p)

./test.sh || exit 1
