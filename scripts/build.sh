#!/bin/bash

set -e

echo "Building MotionEngine..."

# Create build directory if it doesn't exist
mkdir -p build

# Configure and build using CMake
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

echo "Build completed successfully."