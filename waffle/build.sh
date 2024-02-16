#!/bin/bash
set -e

mkdir -p cmakebuild
cd cmakebuild
cmake "$@" -DCMAKE_BUILD_TYPE=Debug ..

START=$(date +%s)
make
#make test ARGS="-VV"
END=$(date +%s)
echo "Total build time (real) = $(( $END - $START )) seconds"