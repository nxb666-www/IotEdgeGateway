#!/bin/bash
set -e

BUILD_TYPE="Debug"

while getopts "r" opt; do
    case $opt in
        r) BUILD_TYPE="Release" ;;
    esac
done

echo "构建类型: $BUILD_TYPE"

cmake -S . -B build-aarch64 -G Ninja \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake \
    -DFETCHCONTENT_SOURCE_DIR_MONGOOSE="$PWD/build/_deps/mongoose-src" \
    -DFETCHCONTENT_SOURCE_DIR_RAPIDYAML="$PWD/build/_deps/rapidyaml-src"

cmake --build build-aarch64

echo "构建完成: build-aarch64/iotgw_gateway"
