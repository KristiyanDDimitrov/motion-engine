#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

./scripts/build.sh

# JUCE puts console apps under <target>_artefacts/<config>/, not build/.
BIN="build/MotionEngineTests_artefacts/Release/MotionEngineTests"
if [ ! -x "$BIN" ]; then
    BIN="$(find build -type f -name MotionEngineTests -perm -111 2>/dev/null | head -n 1 || true)"
fi

if [ -z "${BIN:-}" ] || [ ! -x "$BIN" ]; then
    echo "ERROR: MotionEngineTests binary not found under build/" >&2
    exit 1
fi

echo "Running $BIN ..."
"$BIN"
echo "All tests passed successfully."
