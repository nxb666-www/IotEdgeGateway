#!/bin/bash
set -e

BUILD_TYPE="Debug"

while getopts "r" opt; do
    case $opt in
        r) BUILD_TYPE="Release" ;;
    esac
done

echo "构建类型: $BUILD_TYPE"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE

cmake --build build

echo "构建完成: build/bin/iotgw_gateway"
