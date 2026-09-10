#!/usr/bin/env bash
# =============================================================================
# Start the MotionEngine overnight run.  HARNESS FILE - DO NOT EDIT.
#
#   ./start.sh              up to 60 iterations, 24h wall clock
#   ./start.sh 100          up to 100 iterations
#   MAX_HOURS=8 ./start.sh  stop after 8 hours instead of 24
#
# Runs preflight while you are still watching, then detaches, keeps the Mac
# awake, and runs unattended. Stop it in the morning with ./stop.sh
# =============================================================================
set -uo pipefail
cd "$(dirname "$0")"

ITERS="${1:-60}"

if [ -f .driver.pgid ] && kill -0 "-$(cat .driver.pgid)" 2>/dev/null; then
  echo "A driver is already running (pgid $(cat .driver.pgid)). Run ./stop.sh first."
  exit 1
fi
rm -f .driver.pgid .driver.pid

mkdir -p logs

# Fail loudly now, while you are still at the keyboard, rather than at 3am.
echo "Running preflight (build, tests and one model round trip - a few minutes)..."
echo
if ! ./drive.sh --check 2>&1 | tee logs/preflight.log; then
  echo
  echo "-------------------------------------------------------------------"
  echo "Preflight FAILED. Nothing was started."
  echo "Fix the BAD lines above, then run ./start.sh again."
  echo "Full output: logs/preflight.log"
  echo "-------------------------------------------------------------------"
  exit 1
fi

[ -f run.log ] && mv run.log "logs/run.$(date '+%Y%m%d-%H%M%S').log"
rm -f .driver_done

# Own process group, so ./stop.sh can take down the whole tree in one signal.
set -m
if command -v caffeinate >/dev/null 2>&1; then
  nohup caffeinate -i -m -s ./drive.sh --no-preflight "$ITERS" > run.log 2>&1 &
else
  nohup ./drive.sh --no-preflight "$ITERS" > run.log 2>&1 &
fi
PGID=$!
set +m

echo "$PGID" > .driver.pgid
sleep 2

if ! kill -0 "-$PGID" 2>/dev/null; then
  echo "The driver exited immediately. Last lines of run.log:"
  tail -20 run.log
  rm -f .driver.pgid
  exit 1
fi

cat <<INFO

===================================================================
 MotionEngine is running.
===================================================================
   process group : $PGID
   max iterations: $ITERS
   wall clock    : ${MAX_HOURS:-24}h, then it stops and frees memory
   log           : $PWD/run.log
   per iteration : $PWD/logs/iter-NNN.log

   follow live   : tail -f run.log
   in the morning: ./status.sh     then     ./stop.sh

 The Mac will not sleep while this runs. Leave the lid OPEN - macOS
 sleeps on lid close regardless of caffeinate unless an external
 display is attached.
===================================================================
INFO
