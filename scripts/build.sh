#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# macOS has no nproc. The old `make -j$(nproc)` silently became `make -j`
# (unlimited parallelism), which can wedge the machine building JUCE.
if command -v sysctl >/dev/null 2>&1; then
    JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
elif command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
else
    JOBS=4
fi

echo "Configuring MotionEngine (Release, ${JOBS} jobs)..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build build --config Release --parallel "$JOBS"

echo "Build completed successfully."
