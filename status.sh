#!/usr/bin/env bash
# What happened overnight.
set -uo pipefail
cd "$(dirname "$0")"

echo "=============== MotionEngine status ==============="

if [ -f .driver.pgid ] && kill -0 "-$(cat .driver.pgid)" 2>/dev/null; then
  echo "Driver: RUNNING (pgid $(cat .driver.pgid))"
elif [ -f .driver_done ]; then
  echo "Driver: FINISHED"
  sed 's/^/  /' .driver_done
else
  echo "Driver: not running (no .driver_done - it stopped early, see log tail)"
fi

echo
echo "--- tasks ---"
grep -E '^[[:space:]]*-[[:space:]]*\[' TASKS.md 2>/dev/null | sed 's/^/  /'
echo
echo "  unchecked: $(grep -cE '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null | head -n 1)"
echo "  blocked  : $(grep -c 'BLOCKED' TASKS.md 2>/dev/null | head -n 1)"

echo
echo "--- commits since driver start (last 20) ---"
git --no-pager log --oneline -20 2>/dev/null | sed 's/^/  /'

echo
echo "--- uncommitted changes ---"
git status --porcelain 2>/dev/null | sed 's/^/  /' | head -20

echo
echo "--- STATE.md (last 25 lines) ---"
tail -25 STATE.md 2>/dev/null | sed 's/^/  /'

echo
echo "--- run.log (last 30 lines) ---"
tail -30 run.log 2>/dev/null | sed 's/^/  /'
echo "==================================================="
