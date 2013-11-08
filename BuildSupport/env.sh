# Find base dir
pushd `dirname $0` > /dev/null
BASEDIR=`pwd`
popd > /dev/null

export PATH=$($BASEDIR/../configure.rb --extra-env-paths):$PATH