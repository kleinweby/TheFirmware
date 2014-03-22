#/bin/sh

if [ "$1" = '-p' ]; then
	echo "export PATH=$($(dirname $0)/../configure.rb --extra-env-paths):$PATH"
else
	export PATH=$($(dirname $0)/../configure.rb --extra-env-paths):$PATH
fi

