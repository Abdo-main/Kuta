#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SHADER_DIR="shaders"
TARGET="kuta"

echo "=== Building Kuta ==="

EXE_EXT=""
COPY_CMD="cp"

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Configuring project..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "Compiling..."
cmake --build "$BUILD_DIR" --config Release

echo "Copying shaders..."
$COPY_CMD "$SHADER_DIR"/*.spv "$BUILD_DIR"/

echo -e "\n\033[1;32mBuild successful!\033[0m"
echo "Run with: $BUILD_DIR/$TARGET$EXE_EXT"
