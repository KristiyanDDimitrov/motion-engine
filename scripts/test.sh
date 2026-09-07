#!/usr/bin/env bash
# Build, then run the real test binary.  HARNESS FILE - DO NOT EDIT.
#
# Exit codes are meaningful and the driver depends on them:
#   0  every registered test passed
#   1  at least one assertion failed
#   2  built and ran, but only the harness self test is registered
#      (i.e. no real coverage yet - not a pass)
#   3  build or configure failed
#   4  test binary not found
set -uo pipefail
cd "$(dirname "$0")/.."

./scripts/build.sh || exit 3

# JUCE puts console apps under <target>_artefacts/<config>/, not build/.
BIN="build/MotionEngineTests_artefacts/Release/MotionEngineTests"
if [ ! -x "$BIN" ]; then
    BIN="$(find build -type f -name MotionEngineTests -perm -111 2>/dev/null | head -n 1 || true)"
fi

if [ -z "${BIN:-}" ] || [ ! -x "$BIN" ]; then
    echo "ERROR: MotionEngineTests binary not found under build/" >&2
    exit 4
fi

echo "Running $BIN ..."
"$BIN"
rc=$?

case "$rc" in
    0) echo "scripts/test.sh: PASS" ;;
    1) echo "scripts/test.sh: FAIL - assertions failed" >&2 ;;
    2) echo "scripts/test.sh: NO TESTS - the suite is empty, add Tests/Test<Component>.cpp" >&2 ;;
    *) echo "scripts/test.sh: test binary exited $rc" >&2 ;;
esac
exit $rc
