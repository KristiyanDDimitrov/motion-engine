#!/bin/bash

set -e

echo "Running tests for MotionEngine..."

# Build the test executable first
./scripts/build.sh

# Run the tests
echo "Running MotionEngineTests..."
build/MotionEngineTests

echo "All tests passed successfully."