#!/usr/bin/env bash
# Usage: ./scripts/validate.sh [strictness]   (default 5; T-13 uses 10)
set -euo pipefail
cd "$(dirname "$0")/.."

STRICTNESS="${1:-5}"

./scripts/build.sh

VST3="build/MotionEngine_artefacts/Release/VST3/MotionEngine.vst3"
if [ ! -e "$VST3" ]; then
    VST3="$(find build -maxdepth 6 -name 'MotionEngine.vst3' 2>/dev/null | head -n 1 || true)"
fi
if [ -z "${VST3:-}" ] || [ ! -e "$VST3" ]; then
    echo "ERROR: MotionEngine.vst3 not found under build/" >&2
    exit 1
fi

if command -v pluginval >/dev/null 2>&1; then
    PV="pluginval"
elif [ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    PV="/Applications/pluginval.app/Contents/MacOS/pluginval"
else
    echo "ERROR: pluginval not found. Install it (brew install --cask pluginval)." >&2
    exit 1
fi

echo "Validating $VST3 at strictness $STRICTNESS ..."
"$PV" --strictness-level "$STRICTNESS" --skip-gui-tests --validate "$VST3"
echo "Validation completed successfully."
