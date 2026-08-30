#!/bin/bash

set -e

echo "Building MotionEngine..."

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo "Build completed successfully!"