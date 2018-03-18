#!/bin/bash

. /hbb_exe/activate

usage() { echo "$0 usage:" && grep " .)\ #" $0; exit 0; }

CLEAN=1
BUILD_TYPE="RelWithDebInfo"
SANITIZE=""

while getopts "hcj:ds" o; do
    case "${o}" in
        c) # Clean build - remove Binaries and re-execute cmake
            CLEAN=0
            ;;
        h | *) # Display help
            usage
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

CC="gcc -m32"
CXX="g++ -m32"

if [ ${CLEAN} == 0 ]; then
    make clean
    if [ -d ./CMakeFiles ]; then
        echo "Removing CMakeFiles"
        rm -rf ./CMakeFiles;
    fi

    if [ -e ./CMakeCache.txt ]; then
        echo "Removing CMakeCache.txt"
        rm ./CMakeCache.txt;
    fi
fi

cmake -D CMAKE_BUILD_TYPE=$BUILD_TYPE .

make all
