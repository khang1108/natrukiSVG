#!/bin/bash

# Build and Run Script for NaTruKi Project

set -e  # Exit on error

echo "=== Configuring CMake ==="
cmake -S . -B build

echo ""
echo "=== Building Project ==="
cmake --build build -j$(nproc)

echo ""
echo "=== Checking if executable exists ==="
if [ -f "build/NaTruKi" ]; then
    echo "✓ Build successful! Executable: build/NaTruKi"
    echo ""
    echo "=== Running Application ==="
    ./build/NaTruKi
else
    echo "✗ Build failed - executable not found"
    exit 1
fi

