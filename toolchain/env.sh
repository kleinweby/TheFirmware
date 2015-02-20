#/bin/sh

PATH="$PWD/$(dirname $0)/arm.toolchain/bin:$PWD/$(dirname $0)/arm-gcc.toolchain/bin:$PATH"

if [ "$1" = '-p' ]; then
	echo "export PATH=$PATH"
else
	export PATH
fi

