#!/usr/bin/env bash
# Stop the driver cleanly. Finishes nothing in progress - the current
# claude invocation is killed, but everything already committed is safe.

set -uo pipefail
cd "$(dirname "$0")"

stopped=0

if [ -f .driver.pgid ]; then
  PGID="$(cat .driver.pgid)"
  if kill -0 "-$PGID" 2>/dev/null; then
    echo "Sending TERM to process group $PGID..."
    kill -TERM "-$PGID" 2>/dev/null
    for _ in 1 2 3 4 5; do
      kill -0 "-$PGID" 2>/dev/null || break
      sleep 1
    done
    if kill -0 "-$PGID" 2>/dev/null; then
      echo "Still alive - sending KILL."
      kill -KILL "-$PGID" 2>/dev/null
    fi
    stopped=1
  fi
  rm -f .driver.pgid
fi

# belt and braces
pkill -f 'drive\.sh' 2>/dev/null && stopped=1
pkill -f 'caffeinate -is' 2>/dev/null
rm -f .driver.pid

if [ "$stopped" -eq 1 ]; then
  echo "Driver stopped."
else
  echo "No driver was running."
fi

echo
echo "Repo state:"
echo "  HEAD           : $(git rev-parse --short HEAD 2>/dev/null || echo none)"
echo "  tasks unchecked: $(grep -cE '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null | head -n 1)"
echo "  uncommitted    : $(git status --porcelain 2>/dev/null | wc -l | tr -d ' ') file(s)"
