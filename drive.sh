#!/usr/bin/env bash
# MotionEngine autonomous driver loop.
#
# Completion is decided HERE, by counting unchecked boxes in TASKS.md.
# Nothing the agent writes in STATE.md can start or stop this loop.
#
# Usage:
#   ./drive.sh            # up to 60 iterations
#   ./drive.sh 100        # up to 100 iterations
#   ./drive.sh --check    # run preflight only, then exit
#
# Env overrides:
#   ITER_TIMEOUT     seconds per iteration        (default 5400 = 90 min)
#   MAX_CONSEC_FAIL  abort after N failures       (default 5)
#   MAX_STALLED      abort after N no-progress    (default 3)
#   COOLDOWN         seconds between iterations   (default 5)
#   MODEL            model name                   (default from .env)

set -uo pipefail
cd "$(dirname "$0")"

MAX_ITER="${1:-60}"
ITER_TIMEOUT="${ITER_TIMEOUT:-5400}"
MAX_CONSEC_FAIL="${MAX_CONSEC_FAIL:-5}"
MAX_STALLED="${MAX_STALLED:-3}"
COOLDOWN="${COOLDOWN:-5}"

log() { printf '%s | %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*"; }
die() { log "FATAL: $*"; exit 1; }

# ---------------------------------------------------------------- environment

[ -f ./.env.motionengine ] || die ".env.motionengine not found in $PWD"
set -a
# shellcheck disable=SC1091
source ./.env.motionengine
set +a

MODEL="${MODEL:-${ANTHROPIC_MODEL:-}}"
[ -n "$MODEL" ] || die "No model set. Put ANTHROPIC_MODEL in .env.motionengine."

# timeout binary: macOS needs coreutils (gtimeout)
if command -v gtimeout >/dev/null 2>&1;  then TIMEOUT_BIN="gtimeout"
elif command -v timeout >/dev/null 2>&1; then TIMEOUT_BIN="timeout"
else TIMEOUT_BIN=""; fi

# ------------------------------------------------------------------ preflight

preflight() {
  local bad=0
  log "--- preflight ---"

  for f in PROMPT.md TASKS.md STATE.md CLAUDE.md CMakeLists.txt; do
    if [ -s "$f" ]; then log "  ok    $f"
    else log "  BAD   $f missing or empty"; bad=1; fi
  done

  for s in scripts/build.sh scripts/test.sh scripts/validate.sh; do
    if [ -x "$s" ]; then log "  ok    $s"
    else log "  BAD   $s missing or not executable (chmod +x)"; bad=1; fi
  done

  if [ -f JUCE/CMakeLists.txt ]; then log "  ok    JUCE submodule present"
  else log "  BAD   JUCE/ not checked out (git submodule update --init --recursive)"; bad=1; fi

  for c in claude cmake git; do
    if command -v "$c" >/dev/null 2>&1; then log "  ok    $c on PATH"
    else log "  BAD   $c not on PATH"; bad=1; fi
  done

  if git rev-parse --git-dir >/dev/null 2>&1; then log "  ok    git repo"
  else log "  BAD   not a git repo"; bad=1; fi

  if git config user.email >/dev/null 2>&1 && git config user.name >/dev/null 2>&1; then
    log "  ok    git identity: $(git config user.name) <$(git config user.email)>"
  else
    log "  BAD   git user.name / user.email not configured -> commits will fail"; bad=1
  fi

  if [ -n "${ANTHROPIC_BASE_URL:-}" ]; then log "  ok    ANTHROPIC_BASE_URL=$ANTHROPIC_BASE_URL"
  else log "  BAD   ANTHROPIC_BASE_URL unset"; bad=1; fi

  if [ -n "${CLAUDE_CODE_MAX_CONTEXT_TOKENS:-}" ]; then
    log "  ok    CLAUDE_CODE_MAX_CONTEXT_TOKENS=$CLAUDE_CODE_MAX_CONTEXT_TOKENS"
  else
    log "  BAD   CLAUDE_CODE_MAX_CONTEXT_TOKENS unset -> Claude Code will assume 200k and overrun the model"; bad=1
  fi

  if [ -n "$TIMEOUT_BIN" ]; then log "  ok    timeout binary: $TIMEOUT_BIN"
  else log "  warn  no timeout binary (brew install coreutils) - a hung run will block the night"; fi

  if command -v pluginval >/dev/null 2>&1 || [ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    log "  ok    pluginval found"
  else
    log "  warn  pluginval not found - T-12/T-13 will fail and get BLOCKED"
  fi

  for f in MEMORY.md NOTES.md; do
    [ -f "$f" ] && { log "  warn  $f exists and should not - deleting"; rm -f "$f"; }
  done

  # Generous timeout: a local model that isn't already resident in memory
  # (e.g. evicted by another Ollama client, or a genuinely cold start) can
  # legitimately take well over a minute to load before it answers at all.
  # This is checking "does it eventually work", not "is it fast".
  local smoke_timeout="${SMOKE_TIMEOUT:-240}"
  log "  smoke test: one round trip to $MODEL (timeout ${smoke_timeout}s) ..."
  local out t0 t1
  t0=$(date +%s)
  out="$( { ${TIMEOUT_BIN:+$TIMEOUT_BIN $smoke_timeout} claude --model "$MODEL" --print "Reply with exactly: PONG" ; } 2>&1 )"
  local rc=$?
  t1=$(date +%s)
  if printf '%s' "$out" | grep -q "PONG"; then
    log "  ok    model responded in $((t1 - t0))s"
  elif [ "$rc" -eq 124 ]; then
    log "  BAD   no response within ${smoke_timeout}s (took $((t1 - t0))s then killed)."
    log "        This looks like a slow/cold model load, not a broken endpoint."
    log "        Check: ollama ps   (is another client holding a different model loaded?)"
    log "        Try:   time ollama run $MODEL \"hi\"   to see the real cold-start cost."
    bad=1
  else
    log "  BAD   model did not respond correctly ($((t1 - t0))s). Output was:"
    printf '%s\n' "$out" | sed 's/^/        /'
    bad=1
  fi

  log "--- preflight $( [ $bad -eq 0 ] && echo PASSED || echo FAILED ) ---"
  return $bad
}

# ------------------------------------------------------------ task accounting

# grep -c prints "0" AND exits 1 when there are no matches, so a plain
# `|| echo 0` fallback would emit two lines. Take the first line only.
remaining() {
  local n
  n="$(grep -cE '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null | head -n 1)"
  echo "${n:-0}"
}
blocked() {
  local n
  n="$(grep -c 'BLOCKED' TASKS.md 2>/dev/null | head -n 1)"
  echo "${n:-0}"
}
head_sha() { git rev-parse --short HEAD 2>/dev/null || echo none; }

# Files the agent must never modify. It has broad filesystem access via
# --dangerously-skip-permissions, so this is enforced here, not just asked
# for in the prompt.
HARNESS_FILES="drive.sh start.sh stop.sh status.sh .env.motionengine"

# Revert any uncommitted change to a tracked file, EXCEPT STATE.md — an
# iteration that ran out of time may still have left a useful diagnostic
# line there (rule 8 relies on that history surviving even a failed run).
# Untracked/gitignored files (build/, run.log, .env.motionengine, the
# .driver.* files) are never touched by this.
reset_uncommitted_work() {
  if [ -z "$(git status --porcelain 2>/dev/null)" ]; then return; fi
  log "note: reverting uncommitted changes from the last iteration (it did not commit)"
  cp -f STATE.md /tmp/motionengine_state.md.bak 2>/dev/null || true
  git reset --hard HEAD >/dev/null 2>&1
  if [ -f /tmp/motionengine_state.md.bak ]; then
    cp -f /tmp/motionengine_state.md.bak STATE.md
    rm -f /tmp/motionengine_state.md.bak
  fi
}

# If a commit DID land, make sure it didn't touch a harness file. Auto-fixing
# a bad commit is riskier than just refusing to continue on one — surface it
# and stop rather than silently rewriting history.
check_harness_untouched() {
  local from="$1" to="$2"
  local hit
  hit="$(git diff --name-only "$from" "$to" -- $HARNESS_FILES 2>/dev/null)"
  if [ -n "$hit" ]; then
    log "ABORT: commit $to modified a harness file it must never touch:"
    printf '%s\n' "$hit" | sed 's/^/        /'
    log "Not continuing unattended. Inspect with: git show $to -- $hit"
    return 1
  fi
  return 0
}

# Defensive: the agent is told never to write a completion sentinel, but one
# already broke a previous run. Strip it if it reappears.
scrub_state() {
  if grep -qiE 'ALL TASKS COMPLETE|PROJECT COMPLETE' STATE.md 2>/dev/null; then
    log "note: stripped a completion sentinel the agent wrote into STATE.md"
    grep -viE 'ALL TASKS COMPLETE|PROJECT COMPLETE' STATE.md > STATE.md.tmp && mv STATE.md.tmp STATE.md
  fi
  rm -f MEMORY.md NOTES.md
}

# ----------------------------------------------------------------- final gate

final_verification() {
  log "=== all boxes ticked - running final verification ==="
  local ok=0
  if ./scripts/test.sh; then log "final: tests PASSED"; else log "final: tests FAILED"; ok=1; fi
  if ./scripts/validate.sh 10; then log "final: pluginval(10) PASSED"; else log "final: pluginval(10) FAILED"; ok=1; fi
  {
    echo "finished_at=$(date '+%Y-%m-%d %H:%M:%S')"
    echo "iterations=$1"
    echo "head=$(head_sha)"
    echo "verification=$( [ $ok -eq 0 ] && echo pass || echo fail )"
    echo "blocked_tasks=$(blocked)"
  } > .driver_done
  log "wrote .driver_done"
  if command -v osascript >/dev/null 2>&1; then
    osascript -e "display notification \"MotionEngine finished ($( [ $ok -eq 0 ] && echo pass || echo fail ))\" with title \"drive.sh\"" >/dev/null 2>&1
  fi
}

# ---------------------------------------------------------------------- main

trap 'log "driver received signal - exiting"; rm -f .driver.pid; exit 130' INT TERM

if [ "$MAX_ITER" = "--check" ]; then preflight; exit $?; fi

echo $$ > .driver.pid
rm -f .driver_done

log "############ driver start: max_iter=$MAX_ITER model=$MODEL ############"
preflight || die "preflight failed - fix the BAD lines above and re-run"

fails=0
stalled=0
prev_sha="$(head_sha)"
prev_left="$(remaining)"

for i in $(seq 1 "$MAX_ITER"); do
  scrub_state
  left="$(remaining)"
  blk="$(blocked)"

  if [ "$left" -eq 0 ]; then
    final_verification "$i"
    break
  fi

  log "=== iteration $i/$MAX_ITER | $left task(s) left | $blk blocked | HEAD $(head_sha) ==="

  prompt="$(cat PROMPT.md)"
  if [ -z "$prompt" ]; then die "PROMPT.md is empty - the agent has nothing to do"; fi

  if ${TIMEOUT_BIN:+$TIMEOUT_BIN $ITER_TIMEOUT} \
       claude --model "$MODEL" --dangerously-skip-permissions --print "$prompt"; then
    fails=0
  else
    rc=$?
    fails=$((fails + 1))
    [ "$rc" -eq 124 ] && log "iteration $i TIMED OUT after ${ITER_TIMEOUT}s"
    log "iteration $i exited $rc (consecutive failures: $fails/$MAX_CONSEC_FAIL)"
    if [ "$fails" -ge "$MAX_CONSEC_FAIL" ]; then
      log "ABORT: $MAX_CONSEC_FAIL consecutive failures. Something is wrong with the endpoint or the repo."
      break
    fi
  fi

  new_sha="$(head_sha)"; new_left="$(remaining)"

  if [ "$new_sha" != "$prev_sha" ]; then
    if ! check_harness_untouched "$prev_sha" "$new_sha"; then
      break
    fi
    stalled=0
  else
    # No commit landed. Nothing this iteration touched should persist,
    # including any uncommitted TASKS.md edit — a ticked box with no
    # matching commit is exactly the failure mode that broke an earlier run.
    stalled=$((stalled + 1))
    log "no commit this iteration (stalled $stalled/$MAX_STALLED)"
    reset_uncommitted_work
    new_left="$(remaining)"
    if [ "$stalled" -ge "$MAX_STALLED" ]; then
      log "ABORT: $MAX_STALLED iterations with zero progress. Check STATE.md."
      break
    fi
  fi
  prev_sha="$new_sha"; prev_left="$new_left"

  sleep "$COOLDOWN"
done

log "############ driver stop: $(remaining) task(s) unchecked, $(blocked) blocked, HEAD $(head_sha) ############"
rm -f .driver.pid
