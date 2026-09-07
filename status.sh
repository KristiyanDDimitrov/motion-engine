#!/usr/bin/env bash
# What happened overnight.  HARNESS FILE - DO NOT EDIT.
set -uo pipefail
cd "$(dirname "$0")"

echo "================= MotionEngine status ================="

if [ -f .driver.pgid ] && kill -0 "-$(cat .driver.pgid)" 2>/dev/null; then
  echo "Driver : RUNNING (pgid $(cat .driver.pgid))"
  if [ -f run.log ]; then
    started="$(head -n 1 run.log | cut -d'|' -f1)"
    echo "         started $started"
  fi
elif [ -f .driver_done ]; then
  echo "Driver : FINISHED"
  sed 's/^/         /' .driver_done
else
  echo "Driver : not running, and no .driver_done - it stopped early (see the log tail)"
fi

echo
echo "--- tasks ---"
grep -E '^[[:space:]]*-[[:space:]]*\[' TASKS.md 2>/dev/null | sed 's/^/  /'
echo
echo "  unchecked: $(grep -cE '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null | head -n 1)"
echo "  blocked  : $(grep -c 'BLOCKED' TASKS.md 2>/dev/null | head -n 1)"

echo
echo "--- what the driver decided (iterations, rollbacks, blocks) ---"
grep -E '=== iteration|verification (PASSED|FAILED)|BLOCKING|REJECTED|recovery L|ABORT|driver (start|stop)|hard timeout|went silent' run.log 2>/dev/null \
  | tail -40 | sed 's/^/  /'

echo
echo "--- commits (last 20) ---"
git --no-pager log --oneline -20 2>/dev/null | sed 's/^/  /'

echo
echo "--- uncommitted changes ---"
git status --porcelain 2>/dev/null | sed 's/^/  /' | head -20

echo
echo "--- test suite right now ---"
BIN="$(find build -type f -name MotionEngineTests -perm -111 2>/dev/null | head -n 1 || true)"
if [ -n "${BIN:-}" ] && [ -x "$BIN" ]; then
  "$BIN" 2>&1 | tail -12 | sed 's/^/  /'
else
  echo "  (not built - run ./scripts/test.sh)"
fi

echo
echo "--- STATE.md (last 25 lines) ---"
tail -25 STATE.md 2>/dev/null | sed 's/^/  /'

echo
echo "--- run.log (last 25 lines) ---"
tail -25 run.log 2>/dev/null | sed 's/^/  /'

LAST_ITER="$(ls -1t logs/iter-*.log 2>/dev/null | head -n 1 || true)"
if [ -n "${LAST_ITER:-}" ]; then
  echo
  echo "--- $LAST_ITER (last 10 lines, truncated) ---"
  tail -10 "$LAST_ITER" | cut -c1-200 | sed 's/^/  /'
fi

echo
echo "--- memory ---"
command -v ollama >/dev/null 2>&1 && ollama ps 2>/dev/null | sed 's/^/  /'
sysctl -n vm.swapusage 2>/dev/null | sed 's/^/  swap: /'
echo "======================================================="
