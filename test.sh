#!/bin/bash

./configure.rb -b test || exit 1
ninja || exit 1
make -C tools/simulator || exit 1
tools/simulator/simulator -f firmware.elf