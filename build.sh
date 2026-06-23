#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_TYPE="Debug"
TARGET="native"
CLEAN=0

usage() {
    cat <<EOF
Usage: ./build.sh [options]

Options:
  -x    Native debug build for local development
  -a    ARM64 build for RK3568
  -r    Release build
  -c    Clean build directories
  -h    Show this help

Examples:
  ./build.sh -x
  ./build.sh -a
  ./build.sh -a -r
  ./build.sh -c
EOF
}

while getopts "xarch" opt; do
    case "$opt" in
        x) TARGET="native" ;;
        a) TARGET="aarch64" ;;
        r) BUILD_TYPE="Release" ;;
        c) CLEAN=1 ;;
        h) usage; exit 0 ;;
        *) usage; exit 1 ;;
    esac
done

if [ "$CLEAN" -eq 1 ]; then
    rm -rf "$ROOT_DIR/build/native" "$ROOT_DIR/build/aarch64" \
           "$ROOT_DIR/build-aarch64" "$ROOT_DIR/build/CMakeFiles" \
           "$ROOT_DIR/build/CMakeCache.txt"
    echo "Cleaned build directories"
    exit 0
fi

if [ "$TARGET" = "native" ]; then
    BUILD_DIR="$ROOT_DIR/build/native/$(printf "%s" "$BUILD_TYPE" | tr 'A-Z' 'a-z')"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
else
    BUILD_DIR="$ROOT_DIR/build/aarch64/$(printf "%s" "$BUILD_TYPE" | tr 'A-Z' 'a-z')"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchain-aarch64.cmake" \
        -DFETCHCONTENT_SOURCE_DIR_MONGOOSE="$ROOT_DIR/build/_deps/mongoose-src" \
        -DFETCHCONTENT_SOURCE_DIR_RAPIDYAML="$ROOT_DIR/build/_deps/rapidyaml-src"
fi

cmake --build "$BUILD_DIR"

if [ "$BUILD_TYPE" = "Release" ] && command -v strip >/dev/null 2>&1; then
    strip "$BUILD_DIR/iotgw_gateway" 2>/dev/null || true
    strip "$BUILD_DIR/iotgw_mqtt_broker" 2>/dev/null || true
fi

echo "Build done: $BUILD_DIR"
