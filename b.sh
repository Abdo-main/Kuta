#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SHADER_DIR="shaders"
TARGET="kuta"
CI_MODE=${CI:-false}

echo "=== Building Kuta ==="

# Detect if running in CI
if [ "$CI_MODE" = "true" ]; then
    echo "Running in CI mode"
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON"
else
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Configuring project..."
cmake -S . -B "$BUILD_DIR" $CMAKE_ARGS

echo "Compiling..."
cmake --build "$BUILD_DIR" --config Release --parallel $(nproc)

# Verify shader copying worked
if [ -d "$SHADER_DIR" ]; then
    echo "Verifying shader files..."
    ls -la "$BUILD_DIR"/*.spv 2>/dev/null || echo "Warning: No .spv files found in build directory"
fi

echo -e "\n\033[1;32mBuild successful!\033[0m"
echo "Run with: $BUILD_DIR/$TARGET"

# CI-specific actions
if [ "$CI_MODE" = "true" ]; then
    echo "=== CI Build Information ==="
    file "$BUILD_DIR/$TARGET"
    ls -la "$BUILD_DIR"
fi
