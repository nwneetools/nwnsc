#!/bin/bash

usage() { echo "$0 usage:" && grep " .)\ #" $0; exit 0; }

CLEAN=

while getopts ":hcj:" o; do
    case "${o}" in
        c) # Clean build - remove Binaries and re-execute cmake
            CLEAN=-c
            ;;
        h | *) # Display help
            usage
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

docker run --rm -w /src --entrypoint "/bin/bash" -v $(pwd):/src glorwinger/holy-build-box-32:1.0.0 ./scripts/buildnwnsc.sh ${CLEAN}
