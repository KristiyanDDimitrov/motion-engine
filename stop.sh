#!/usr/bin/env bash
# =============================================================================
# Stop the run and give the memory back.  HARNESS FILE - DO NOT EDIT.
#
#   ./stop.sh                 stop the driver, unload the model from RAM
#   ./stop.sh --quit-ollama   also quit the Ollama server entirely
#
# Anything already committed is safe. The iteration in flight is discarded.
# =============================================================================
set -uo pipefail
cd "$(dirname "$0")"

QUIT_OLLAMA=0
[ "${1:-}" = "--quit-ollama" ] && QUIT_OLLAMA=1

OLLAMA_URL="${OLLAMA_URL:-http://localhost:11434}"
PRIMARY_MODEL=""
if [ -f .env.motionengine ]; then
  PRIMARY_MODEL="$(awk -F= '/^export ANTHROPIC_MODEL=/{print $2}' .env.motionengine | tr -d '"'"'"' ' | head -n 1)"
fi
FALLBACK_MODEL="${FALLBACK_MODEL:-motionengine-fb32k}"

stopped=0

# ---------------------------------------------------------------- the driver

if [ -f .driver.pgid ]; then
  PGID="$(cat .driver.pgid)"
  if kill -0 "-$PGID" 2>/dev/null; then
    echo "Stopping driver process group $PGID ..."
    kill -TERM "-$PGID" 2>/dev/null
    for _ in 1 2 3 4 5 6 7 8 9 10; do
      kill -0 "-$PGID" 2>/dev/null || break
      sleep 1
    done
    if kill -0 "-$PGID" 2>/dev/null; then
      echo "  still alive - sending KILL"
      kill -KILL "-$PGID" 2>/dev/null
    fi
    stopped=1
  fi
  rm -f .driver.pgid
fi

if [ -f .driver.pid ]; then
  DPID="$(cat .driver.pid)"
  kill -TERM "$DPID" 2>/dev/null && stopped=1
  sleep 1
  kill -KILL "$DPID" 2>/dev/null
  rm -f .driver.pid
fi

# Only ever touch the caffeinate this harness started, never a stray one.
pkill -f 'caffeinate -i -m -s ./drive.sh' >/dev/null 2>&1
pkill -f 'drive\.sh' >/dev/null 2>&1 && stopped=1
# The agent process is identified by the flag only this harness passes it.
pkill -f 'claude --model .* --dangerously-skip-permissions' >/dev/null 2>&1

# ------------------------------------------------------- give the RAM back

unload() {
  local m="$1"
  [ -n "$m" ] || return 0
  if command -v ollama >/dev/null 2>&1 && ollama stop "$m" >/dev/null 2>&1; then
    echo "  unloaded $m"
    return 0
  fi
  if curl -fsS --max-time 20 "$OLLAMA_URL/api/generate" \
          -d "{\"model\":\"$m\",\"keep_alive\":0}" >/dev/null 2>&1; then
    echo "  unloaded $m (via API)"
  fi
  return 0
}

echo "Releasing model memory..."
unload "$PRIMARY_MODEL"
unload "$FALLBACK_MODEL"

if [ "$QUIT_OLLAMA" = "1" ]; then
  echo "Quitting Ollama..."
  osascript -e 'quit app "Ollama"' >/dev/null 2>&1
  pkill -f 'ollama serve' >/dev/null 2>&1
  killall Ollama >/dev/null 2>&1
fi

if command -v ollama >/dev/null 2>&1; then
  echo
  echo "Still resident in memory (should be empty):"
  ollama ps 2>/dev/null | sed 's/^/  /'
fi

# ---------------------------------------------------------------- what we got

echo
if [ "$stopped" -eq 1 ]; then echo "Driver stopped."; else echo "No driver was running."; fi
echo
echo "Repo state:"
echo "  HEAD           : $(git rev-parse --short HEAD 2>/dev/null || echo none)"
echo "  tasks unchecked: $(grep -cE '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null | head -n 1)"
echo "  tasks blocked  : $(grep -c 'BLOCKED' TASKS.md 2>/dev/null | head -n 1)"
echo "  uncommitted    : $(git status --porcelain 2>/dev/null | wc -l | tr -d ' ') file(s)"
echo
echo "Run ./status.sh for the full picture of the night."
