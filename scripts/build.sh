#!/usr/bin/env bash
# Configure + build Release.  HARNESS FILE - DO NOT EDIT.
#
#   ./scripts/build.sh            incremental build
#   ./scripts/build.sh --clean    wipe build/ first
#
# Exit codes: 0 ok, 3 build failed.
set -uo pipefail
cd "$(dirname "$0")/.."

if [ "${1:-}" = "--clean" ]; then
    echo "Removing build/ ..."
    rm -rf build
fi

# macOS has no nproc. Parallelism is deliberately capped: a full-throttle JUCE
# build next to a ~20GB resident model on a 24GB machine pushes it into swap,
# which is the failure mode that has cost whole nights before.
if [ -n "${BUILD_JOBS:-}" ]; then
    JOBS="$BUILD_JOBS"
elif command -v sysctl >/dev/null 2>&1; then
    NCPU="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    JOBS=$(( NCPU > 6 ? 6 : NCPU ))
elif command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
else
    JOBS=4
fi

echo "Configuring MotionEngine (Release, ${JOBS} jobs)..."
if ! cmake -S . -B build -DCMAKE_BUILD_TYPE=Release; then
    echo "ERROR: cmake configure failed." >&2
    exit 3
fi

echo "Building..."
if ! cmake --build build --config Release --parallel "$JOBS"; then
    echo "ERROR: build failed." >&2
    exit 3
fi

echo "Build completed successfully."
