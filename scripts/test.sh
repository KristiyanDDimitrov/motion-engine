#!/bin/bash

set -e

echo "Running tests for MotionEngine..."

# Build the tests first
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run the test executable
./MotionEngineTests

echo "All tests passed!"