#!/usr/bin/env bash
# Start the MotionEngine driver in the background, keep the Mac awake,
# and detach so you can close the terminal / go to bed.
#
#   ./start.sh          # up to 60 iterations
#   ./start.sh 100      # up to 100 iterations

set -euo pipefail
cd "$(dirname "$0")"

ITERS="${1:-60}"

if [ -f .driver.pgid ] && kill -0 "-$(cat .driver.pgid)" 2>/dev/null; then
  echo "Driver already running (pgid $(cat .driver.pgid)). Use ./stop.sh first."
  exit 1
fi

# Fail fast before detaching, so problems are visible while you are still here.
echo "Running preflight..."
if ! ./drive.sh --check; then
  echo
  echo "Preflight FAILED. Not starting. Fix the BAD lines above."
  exit 1
fi

# rotate logs
[ -f run.log ] && mv run.log "run.$(date '+%Y%m%d-%H%M%S').log"
rm -f .driver_done

set -m   # give the background job its own process group
if command -v caffeinate >/dev/null 2>&1; then
  nohup caffeinate -is ./drive.sh "$ITERS" > run.log 2>&1 &
else
  nohup ./drive.sh "$ITERS" > run.log 2>&1 &
fi
PGID=$!
set +m

echo "$PGID" > .driver.pgid
sleep 1

echo
echo "Driver started."
echo "  process group : $PGID"
echo "  max iterations: $ITERS"
echo "  log           : $PWD/run.log"
echo
echo "  follow live   : tail -f run.log"
echo "  check status  : ./status.sh"
echo "  stop          : ./stop.sh"
