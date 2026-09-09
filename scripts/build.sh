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

# ---------------------------------------------------------------------------
# The model must not be resident while clang runs.
#
# The primary model is 18GB of weights plus a 64k KV cache, on a machine with
# 24GB of unified memory. That leaves roughly nothing for the compiler, and the
# whole night is then spent in swap - which is what wedged the machine on the
# 2026-09-09 run. Ollama reloads the model automatically on the next inference
# call, so the only cost here is one cold start (~90s) per build.
#
# Set KEEP_MODEL_LOADED=1 to skip this, e.g. when building by hand.
# ---------------------------------------------------------------------------
FREED=0
if [ "${KEEP_MODEL_LOADED:-0}" != "1" ] && command -v ollama >/dev/null 2>&1; then
    RESIDENT="$(ollama ps 2>/dev/null | awk 'NR>1 && NF {print $1}')"
    if [ -n "$RESIDENT" ]; then
        for m in $RESIDENT; do
            echo "Unloading $m so the build has memory to work with..."
            ollama stop "$m" >/dev/null 2>&1 || true
        done
        sleep 3
        FREED=1
    fi
fi

# Parallelism is chosen from what is actually free, not from the core count. A
# full-throttle JUCE build next to a resident model pushes the machine into
# swap, and swap is what kills an overnight run.
if [ -n "${BUILD_JOBS:-}" ]; then
    JOBS="$BUILD_JOBS"
elif command -v vm_stat >/dev/null 2>&1; then
    PS="$(sysctl -n hw.pagesize 2>/dev/null || echo 16384)"
    VM="$(vm_stat 2>/dev/null)"
    F="$(printf '%s\n' "$VM" | awk '/Pages free/        {gsub(/[^0-9]/,"",$3); print $3; exit}')"
    I="$(printf '%s\n' "$VM" | awk '/Pages inactive/    {gsub(/[^0-9]/,"",$3); print $3; exit}')"
    S="$(printf '%s\n' "$VM" | awk '/Pages speculative/ {gsub(/[^0-9]/,"",$3); print $3; exit}')"
    FREE_MB=$(( (( ${F:-0} + ${I:-0} + ${S:-0} ) * PS) / 1048576 ))
    NCPU="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    if   [ "$FREE_MB" -gt 12000 ]; then JOBS=$(( NCPU > 6 ? 6 : NCPU ))
    elif [ "$FREE_MB" -gt  6000 ]; then JOBS=4
    else                                JOBS=2
    fi
    echo "Memory available: ${FREE_MB}MB (model unloaded: $FREED) -> ${JOBS} build jobs"
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
