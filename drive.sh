#!/usr/bin/env bash
# =============================================================================
# MotionEngine autonomous driver loop.  HARNESS FILE - DO NOT EDIT.
#
# Design rules this file exists to enforce:
#   * Completion is decided HERE, by counting unchecked boxes in TASKS.md and
#     by running the tests. Nothing the agent writes can start or stop the loop.
#   * A commit is only kept if ./scripts/test.sh passes AFTER it. Fabricated
#     work gets rolled back automatically.
#   * Failure never ends the night. It steps down a recovery ladder, and a task
#     that fails repeatedly is marked [BLOCKED] so the loop moves on.
#
# Usage:
#   ./drive.sh              up to 60 iterations
#   ./drive.sh 100          up to 100 iterations
#   ./drive.sh --check      preflight only, then exit
#   ./drive.sh --no-preflight 60   skip preflight (start.sh already ran it)
#
# Env overrides:
#   ITER_TIMEOUT   hard seconds per iteration      (default 9000 = 2h30)
#   IDLE_TIMEOUT   seconds of no output -> kill    (default 1500 = 25 min)
#   MAX_HOURS      wall-clock limit for the run    (default 10, 0 = unlimited)
#   MAX_TASK_FAILS attempts before a task is BLOCKED (default 3)
#   MAX_CONSEC_FAIL hard abort after N failures    (default 8)
#   MIN_FREE_MB    free memory required to start   (default 3000)
#   COOLDOWN       seconds between iterations      (default 10)
#   FALLBACK_CTX   context for the fallback model  (default 32768)
#   VERIFY_AFTER_COMMIT  1/0                       (default 1)
# =============================================================================

set -uo pipefail
cd "$(dirname "$0")"

# ------------------------------------------------------------------ arguments

RUN_PREFLIGHT=1
CHECK_ONLY=0
MAX_ITER=""

while [ $# -gt 0 ]; do
  case "$1" in
    --check)         CHECK_ONLY=1 ;;
    --no-preflight)  RUN_PREFLIGHT=0 ;;
    -*)              echo "unknown option: $1" >&2; exit 64 ;;
    *)               MAX_ITER="$1" ;;
  esac
  shift
done
MAX_ITER="${MAX_ITER:-60}"

ITER_TIMEOUT="${ITER_TIMEOUT:-9000}"
IDLE_TIMEOUT="${IDLE_TIMEOUT:-1500}"
MAX_HOURS="${MAX_HOURS:-10}"
MAX_TASK_FAILS="${MAX_TASK_FAILS:-3}"
MAX_CONSEC_FAIL="${MAX_CONSEC_FAIL:-8}"
MIN_FREE_MB="${MIN_FREE_MB:-3000}"
COOLDOWN="${COOLDOWN:-10}"
FALLBACK_CTX="${FALLBACK_CTX:-32768}"
FALLBACK_CC_TOKENS="${FALLBACK_CC_TOKENS:-30000}"
VERIFY_AFTER_COMMIT="${VERIFY_AFTER_COMMIT:-1}"
OLLAMA_URL="${OLLAMA_URL:-http://localhost:11434}"

RUNDIR="logs"
mkdir -p "$RUNDIR"
TASKFAILS="$RUNDIR/.taskfails"
touch "$TASKFAILS"

START_TS="$(date +%s)"

log() { printf '%s | %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*"; }
logf() { sed 's/^/                      | /'; }
die() { log "FATAL: $*"; exit 1; }

# ---------------------------------------------------------------- environment

[ -f ./.env.motionengine ] || die ".env.motionengine not found in $PWD"
set -a
# shellcheck disable=SC1091
. ./.env.motionengine
set +a

PRIMARY_MODEL="${MODEL:-${ANTHROPIC_MODEL:-}}"
[ -n "$PRIMARY_MODEL" ] || die "No model set. Put ANTHROPIC_MODEL in .env.motionengine."
PRIMARY_CC_TOKENS="${CLAUDE_CODE_MAX_CONTEXT_TOKENS:-62000}"
FALLBACK_MODEL="${FALLBACK_MODEL:-motionengine-fb32k}"

# Filled in by preflight; safe defaults if preflight is skipped.
STREAM_ARGS="${STREAM_ARGS:-}"
HAVE_FALLBACK=0

# Tracked files the agent must never change. It runs with
# --dangerously-skip-permissions, so this is enforced here, not merely asked for.
HARNESS_TRACKED="drive.sh start.sh stop.sh status.sh doctor.sh PROMPT.md CLAUDE.md Tests/TestMain.cpp scripts/build.sh scripts/test.sh scripts/validate.sh"

# ------------------------------------------------------------- system probes

free_mb() {
  local ps vm f i s
  command -v vm_stat >/dev/null 2>&1 || { echo 999999; return; }
  ps="$(sysctl -n hw.pagesize 2>/dev/null || echo 16384)"
  vm="$(vm_stat 2>/dev/null)" || { echo 999999; return; }
  f="$(printf '%s\n' "$vm" | awk '/Pages free/          {gsub(/[^0-9]/,"",$3); print $3; exit}')"
  i="$(printf '%s\n' "$vm" | awk '/Pages inactive/      {gsub(/[^0-9]/,"",$3); print $3; exit}')"
  s="$(printf '%s\n' "$vm" | awk '/Pages speculative/   {gsub(/[^0-9]/,"",$3); print $3; exit}')"
  f="${f:-0}"; i="${i:-0}"; s="${s:-0}"
  echo $(( ((f + i + s) * ps) / 1048576 ))
}

swap_used_mb() {
  sysctl -n vm.swapusage 2>/dev/null \
    | awk '{for(n=1;n<=NF;n++) if($n=="used"){gsub(/[^0-9.]/,"",$(n+2)); printf "%d", $(n+2); exit}}' \
    || echo 0
}

ollama_up() { curl -fsS --max-time 10 "$OLLAMA_URL/api/tags" >/dev/null 2>&1; }

ollama_unload() {
  local m="$1"
  command -v ollama >/dev/null 2>&1 && ollama stop "$m" >/dev/null 2>&1 && return 0
  curl -fsS --max-time 20 "$OLLAMA_URL/api/generate" \
       -d "{\"model\":\"$m\",\"keep_alive\":0}" >/dev/null 2>&1 || true
}

ollama_unload_all() {
  ollama_unload "$PRIMARY_MODEL"
  [ "$HAVE_FALLBACK" = "1" ] && ollama_unload "$FALLBACK_MODEL"
  return 0
}

ollama_restart() {
  log "restarting Ollama ..."
  ollama_unload_all
  pkill -f 'ollama serve' >/dev/null 2>&1
  osascript -e 'quit app "Ollama"' >/dev/null 2>&1
  killall Ollama >/dev/null 2>&1
  sleep 8
  if [ -d /Applications/Ollama.app ]; then
    open -a Ollama >/dev/null 2>&1
  elif command -v ollama >/dev/null 2>&1; then
    nohup ollama serve >"$RUNDIR/ollama-serve.log" 2>&1 &
  fi
  local n
  for n in $(seq 1 60); do
    ollama_up && { log "  Ollama is back up after $((n * 2))s"; return 0; }
    sleep 2
  done
  log "  Ollama did NOT come back up"
  return 1
}

model_installed() {
  ollama list 2>/dev/null | awk 'NR>1{print $1}' | sed 's/:latest$//' | grep -qx "$1"
}

# Build a reduced-context twin of the primary model, derived from the primary's
# own Modelfile so nothing about it has to be hardcoded here.
ensure_fallback_model() {
  if model_installed "$FALLBACK_MODEL"; then HAVE_FALLBACK=1; return 0; fi
  command -v ollama >/dev/null 2>&1 || return 1
  log "creating fallback model $FALLBACK_MODEL (num_ctx=$FALLBACK_CTX) from $PRIMARY_MODEL ..."
  local mf="$RUNDIR/fallback.modelfile"
  if ! ollama show "$PRIMARY_MODEL" --modelfile > "$mf" 2>"$RUNDIR/ollama-create.log"; then
    log "  cannot read the Modelfile of $PRIMARY_MODEL - the run stays on the primary model"
    return 1
  fi
  grep -viE '^[[:space:]]*PARAMETER[[:space:]]+num_ctx' "$mf" > "$mf.tmp" && mv "$mf.tmp" "$mf"
  printf 'PARAMETER num_ctx %s\n' "$FALLBACK_CTX" >> "$mf"
  if ollama create "$FALLBACK_MODEL" -f "$mf" >>"$RUNDIR/ollama-create.log" 2>&1; then
    log "  created $FALLBACK_MODEL"
    HAVE_FALLBACK=1
    return 0
  fi
  log "  ollama create failed (see $RUNDIR/ollama-create.log) - staying on the primary model"
  return 1
}

# ------------------------------------------------------------ task accounting

# grep -c prints 0 AND exits 1 on no match, so take the first line only.
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

current_task() {
  grep -E '^[[:space:]]*-[[:space:]]*\[[[:space:]]*\]' TASKS.md 2>/dev/null \
    | head -n 1 | sed -E 's/.*(T-[0-9]+).*/\1/'
}

task_fails() {
  local n
  n="$(grep -E "^$1 " "$TASKFAILS" 2>/dev/null | tail -n 1 | awk '{print $2}')"
  echo "${n:-0}"
}
set_task_fails() {
  grep -vE "^$1 " "$TASKFAILS" > "$TASKFAILS.tmp" 2>/dev/null || true
  mv -f "$TASKFAILS.tmp" "$TASKFAILS" 2>/dev/null || touch "$TASKFAILS"
  printf '%s %s\n' "$1" "$2" >> "$TASKFAILS"
}

block_task() {
  local t="$1" why="$2"
  log "BLOCKING $t after $MAX_TASK_FAILS failed attempts - moving on to the next task"
  awk -v t="$t" '{
        if ($0 ~ "^[ \t]*-[ \t]*\\[[ \t]*\\][ \t]*" t) sub(/\[[ \t]*\]/, "[BLOCKED]")
        print
      }' TASKS.md > TASKS.md.tmp && mv TASKS.md.tmp TASKS.md
  {
    echo ""
    echo "$(date '+%Y-%m-%d %H:%M'): DRIVER blocked $t after $MAX_TASK_FAILS attempts."
    echo "Reason: $why. Later tasks that depend on $t may also fail; review by hand."
  } >> STATE.md
  git add TASKS.md STATE.md >/dev/null 2>&1
  git commit -q -m "harness: block $t after $MAX_TASK_FAILS failed attempts" >/dev/null 2>&1 || true
  set_task_fails "$t" 0
}

# ----------------------------------------------------------- repo hygiene

# Revert uncommitted changes to tracked files, EXCEPT STATE.md - a run that
# died may still have left a useful diagnostic line there.
reset_uncommitted_work() {
  [ -z "$(git status --porcelain 2>/dev/null)" ] && return 0
  log "reverting uncommitted changes from the last iteration (it did not commit)"
  cp -f STATE.md "$RUNDIR/state.bak" 2>/dev/null || true
  git reset --hard HEAD >/dev/null 2>&1
  [ -f "$RUNDIR/state.bak" ] && cp -f "$RUNDIR/state.bak" STATE.md
  return 0
}

# If a commit touched a protected harness file, put it back and carry on.
# Aborting the night over this is worse than repairing it.
restore_harness() {
  local from="$1" to="$2" hit
  hit="$(git diff --name-only "$from" "$to" -- $HARNESS_TRACKED 2>/dev/null)"
  [ -z "$hit" ] && return 0
  log "WARNING: commit $to modified protected harness file(s):"
  printf '%s\n' "$hit" | logf
  git checkout "$from" -- $hit >/dev/null 2>&1
  git add $hit >/dev/null 2>&1
  git commit -q -m "harness: restore protected files modified by the agent" >/dev/null 2>&1 || true
  log "         restored from $from and committed - continuing"
  return 0
}

restore_env_file() {
  [ -f "$RUNDIR/env.backup" ] || return 0
  if ! cmp -s "$RUNDIR/env.backup" .env.motionengine; then
    log "WARNING: .env.motionengine was modified - restoring it"
    cp -f "$RUNDIR/env.backup" .env.motionengine
  fi
  return 0
}

scrub_state() {
  if grep -qiE 'ALL TASKS COMPLETE|PROJECT COMPLETE|ALL DONE' STATE.md 2>/dev/null; then
    log "stripped a completion sentinel the agent wrote into STATE.md"
    grep -viE 'ALL TASKS COMPLETE|PROJECT COMPLETE|ALL DONE' STATE.md > STATE.md.tmp && mv STATE.md.tmp STATE.md
  fi
  rm -f MEMORY.md NOTES.md README.md.bak
  rm -rf memory .claude/bootstrap-complete.md .claude/Bootstrap_complete.md
  # Keep STATE.md bounded so it never eats the context window.
  local n
  n="$(wc -l < STATE.md 2>/dev/null | tr -d ' ')"
  if [ -n "$n" ] && [ "$n" -gt 300 ]; then
    log "STATE.md was $n lines - trimming to the most recent 250"
    { head -n 1 STATE.md; echo "";
      echo "[earlier entries trimmed by the driver on $(date '+%Y-%m-%d')]"; echo "";
      tail -n 250 STATE.md; } > STATE.md.tmp && mv STATE.md.tmp STATE.md
  fi
  return 0
}

# ------------------------------------------------------- supervised execution

kill_group() {
  local pid="$1" n
  kill -TERM "-$pid" 2>/dev/null || kill -TERM "$pid" 2>/dev/null
  for n in 1 2 3 4 5 6 7 8 9 10; do
    kill -0 "$pid" 2>/dev/null || return 0
    sleep 1
  done
  kill -KILL "-$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null
  return 0
}

# watched_run <hard_budget_s> <idle_s> <logfile> <command...>
#   returns the command's exit code, or 124 on hard timeout, 125 on idle stall.
# The child is put in its own process group so the whole tree dies with it -
# a killed `claude` that leaves children behind would wedge the next iteration.
watched_run() {
  local budget="$1" idle="$2" lgf="$3"; shift 3
  : > "$lgf"
  (
    if command -v perl >/dev/null 2>&1; then
      exec perl -e 'setpgrp(0,0); exec @ARGV or die "exec failed: $!\n"' -- "$@"
    else
      exec "$@"
    fi
  ) >"$lgf" 2>&1 &
  local pid=$! start now sz last_sz last_change rc
  start="$(date +%s)"; last_sz=-1; last_change="$start"
  while kill -0 "$pid" 2>/dev/null; do
    sleep 10
    now="$(date +%s)"
    sz="$(wc -c < "$lgf" 2>/dev/null | tr -d ' ')"; sz="${sz:-0}"
    if [ "$sz" != "$last_sz" ]; then last_sz="$sz"; last_change="$now"; fi
    if [ "$budget" -gt 0 ] && [ $((now - start)) -ge "$budget" ]; then
      kill_group "$pid"; wait "$pid" 2>/dev/null; return 124
    fi
    if [ "$idle" -gt 0 ] && [ $((now - last_change)) -ge "$idle" ]; then
      kill_group "$pid"; wait "$pid" 2>/dev/null; return 125
    fi
  done
  wait "$pid"; rc=$?; return "$rc"
}

# --------------------------------------------------------- recovery ladder

# level 0  primary model, as configured
# level 1  primary model, after unloading it (clears a swapped-out/wedged load)
# level 2  reduced-context fallback model
# level 3+ Ollama restarted, then the fallback model
LEVEL=0

level_model() { case "$1" in 0|1) echo "$PRIMARY_MODEL" ;; *) [ "$HAVE_FALLBACK" = "1" ] && echo "$FALLBACK_MODEL" || echo "$PRIMARY_MODEL" ;; esac; }
level_ctx()   { case "$1" in 0|1) echo "$PRIMARY_CC_TOKENS" ;; *) [ "$HAVE_FALLBACK" = "1" ] && echo "$FALLBACK_CC_TOKENS" || echo "$PRIMARY_CC_TOKENS" ;; esac; }

level_prepare() {
  case "$1" in
    0) : ;;
    1) log "recovery L1: unloading $PRIMARY_MODEL to clear memory pressure, then retrying"
       ollama_unload_all; sleep 20 ;;
    2) log "recovery L2: stepping down to the reduced-context fallback model"
       ollama_unload_all
       ensure_fallback_model || log "  no fallback available - retrying on the primary model"
       sleep 10 ;;
    *) log "recovery L3: restarting Ollama, then running on the fallback model"
       ollama_restart || log "  Ollama restart did not confirm - trying anyway"
       ensure_fallback_model || true
       sleep 10 ;;
  esac
  return 0
}

memory_guard() {
  local f s n
  f="$(free_mb)"; s="$(swap_used_mb)"
  log "memory: ${f}MB available, ${s}MB swap in use"
  [ "$f" -ge "$MIN_FREE_MB" ] && return 0
  log "  below the ${MIN_FREE_MB}MB floor - unloading models and waiting"
  ollama_unload_all
  for n in 1 2 3 4 5 6; do
    sleep 20
    f="$(free_mb)"
    [ "$f" -ge "$MIN_FREE_MB" ] && { log "  recovered to ${f}MB"; return 0; }
  done
  log "  still only ${f}MB free - continuing on the fallback model rather than stalling"
  [ "$LEVEL" -lt 2 ] && LEVEL=2
  return 0
}

digest() {
  local lgf="$1" n
  n="$(wc -l < "$lgf" 2>/dev/null | tr -d ' ')"
  log "agent output: ${n:-0} line(s) in $lgf"
  tail -n 3 "$lgf" 2>/dev/null | cut -c1-300 | logf
  return 0
}

# ------------------------------------------------------------------ preflight

preflight() {
  local bad=0 rc
  log "--- preflight ---"

  for f in PROMPT.md TASKS.md STATE.md CLAUDE.md CMakeLists.txt Tests/TestMain.cpp; do
    if [ -s "$f" ]; then log "  ok    $f"; else log "  BAD   $f missing or empty"; bad=1; fi
  done
  for s in scripts/build.sh scripts/test.sh scripts/validate.sh; do
    if [ -x "$s" ]; then log "  ok    $s"; else log "  BAD   $s missing or not executable (chmod +x $s)"; bad=1; fi
  done
  if [ -f JUCE/CMakeLists.txt ]; then log "  ok    JUCE submodule present"
  else log "  BAD   JUCE/ not checked out (git submodule update --init --recursive)"; bad=1; fi
  for c in claude cmake git curl awk; do
    if command -v "$c" >/dev/null 2>&1; then log "  ok    $c on PATH"; else log "  BAD   $c not on PATH"; bad=1; fi
  done
  if command -v ollama >/dev/null 2>&1; then log "  ok    ollama on PATH"
  else log "  warn  ollama CLI not on PATH - model unload/fallback will use the HTTP API only"; fi
  if command -v perl >/dev/null 2>&1; then log "  ok    perl (used to kill runaway process trees)"
  else log "  warn  no perl - a killed iteration may leave child processes behind"; fi
  if git rev-parse --git-dir >/dev/null 2>&1; then log "  ok    git repo"; else log "  BAD   not a git repo"; bad=1; fi
  if git config user.email >/dev/null 2>&1 && git config user.name >/dev/null 2>&1; then
    log "  ok    git identity: $(git config user.name) <$(git config user.email)>"
  else log "  BAD   git user.name / user.email not set -> every commit will fail"; bad=1; fi
  if [ -n "${ANTHROPIC_BASE_URL:-}" ]; then log "  ok    ANTHROPIC_BASE_URL=$ANTHROPIC_BASE_URL"
  else log "  BAD   ANTHROPIC_BASE_URL unset"; bad=1; fi
  log "  ok    CLAUDE_CODE_MAX_CONTEXT_TOKENS=$PRIMARY_CC_TOKENS"

  if ollama_up; then log "  ok    Ollama responding at $OLLAMA_URL"
  else
    log "  warn  Ollama not responding - attempting a restart"
    if ollama_restart; then log "  ok    Ollama restarted"; else log "  BAD   Ollama is not reachable"; bad=1; fi
  fi
  if model_installed "$PRIMARY_MODEL"; then log "  ok    model $PRIMARY_MODEL is installed"
  else log "  BAD   model $PRIMARY_MODEL is not in 'ollama list'"; bad=1; fi
  if ensure_fallback_model; then log "  ok    fallback model $FALLBACK_MODEL ready (num_ctx=$FALLBACK_CTX)"
  else log "  warn  no fallback model - recovery levels 2/3 will reuse the primary model"; fi

  log "  memory: $(free_mb)MB available, $(swap_used_mb)MB swap in use"
  if command -v pluginval >/dev/null 2>&1 || [ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    log "  ok    pluginval found"
  else log "  warn  pluginval not found - T-12/T-13 will fail and end up BLOCKED"; fi

  rm -f MEMORY.md NOTES.md
  rm -rf memory .claude/bootstrap-complete.md .claude/Bootstrap_complete.md 2>/dev/null

  # The single most important check: the build and the real test binary. If
  # this is broken, every "tests must pass" gate downstream is meaningless.
  if [ "${SKIP_BUILD_CHECK:-0}" = "1" ]; then
    log "  skip  build/test check (SKIP_BUILD_CHECK=1)"
  else
    log "  build + test check (this can take a few minutes) ..."
    ./scripts/test.sh > "$RUNDIR/preflight-test.log" 2>&1; rc=$?
    case "$rc" in
      0) log "  ok    build passes and the test suite passes" ;;
      2) log "  ok    build passes; suite has no real tests yet (expected before T-02)" ;;
      *) log "  BAD   ./scripts/test.sh exited $rc - see $RUNDIR/preflight-test.log"
         tail -n 25 "$RUNDIR/preflight-test.log" | logf
         bad=1 ;;
    esac
  fi

  # Smoke test the model, and work out whether streaming output works against
  # this endpoint. Streaming is what makes the idle watchdog possible.
  log "  smoke test: one round trip to $PRIMARY_MODEL (a cold load can take minutes) ..."
  local t0 t1
  t0="$(date +%s)"
  watched_run "${SMOKE_TIMEOUT:-420}" 0 "$RUNDIR/smoke-stream.log" \
      claude --model "$PRIMARY_MODEL" --output-format stream-json --verbose \
             --print "Reply with exactly: PONG"
  rc=$?
  t1="$(date +%s)"
  if [ $rc -eq 0 ] && grep -q "PONG" "$RUNDIR/smoke-stream.log" 2>/dev/null; then
    STREAM_ARGS="--output-format stream-json --verbose"
    log "  ok    model responded in $((t1 - t0))s, streaming output works"
    log "        idle watchdog active: an iteration silent for ${IDLE_TIMEOUT}s is killed"
  else
    log "  note  streaming did not work (exit $rc) - retrying without it"
    t0="$(date +%s)"
    watched_run "${SMOKE_TIMEOUT:-420}" 0 "$RUNDIR/smoke-plain.log" \
        claude --model "$PRIMARY_MODEL" --print "Reply with exactly: PONG"
    rc=$?
    t1="$(date +%s)"
    if [ $rc -eq 0 ] && grep -q "PONG" "$RUNDIR/smoke-plain.log" 2>/dev/null; then
      STREAM_ARGS=""
      IDLE_TIMEOUT=0
      log "  ok    model responded in $((t1 - t0))s (non-streaming)"
      log "        idle watchdog disabled - only the ${ITER_TIMEOUT}s hard timeout applies"
    else
      log "  BAD   model did not respond correctly (exit $rc after $((t1 - t0))s). Output:"
      tail -n 20 "$RUNDIR/smoke-plain.log" 2>/dev/null | logf
      bad=1
    fi
  fi

  log "--- preflight $( [ $bad -eq 0 ] && echo PASSED || echo FAILED ) ---"
  return $bad
}

# ----------------------------------------------------------------- final gate

final_verification() {
  log "=== every box is ticked - running final verification ==="
  local ok=0 rc
  ./scripts/test.sh > "$RUNDIR/final-test.log" 2>&1; rc=$?
  if [ $rc -eq 0 ]; then log "final: tests PASSED"; else log "final: tests FAILED (exit $rc)"; ok=1; fi
  ./scripts/validate.sh 10 > "$RUNDIR/final-validate.log" 2>&1
  if [ $? -eq 0 ]; then log "final: pluginval(10) PASSED"; else log "final: pluginval(10) FAILED"; ok=1; fi
  {
    echo "finished_at=$(date '+%Y-%m-%d %H:%M:%S')"
    echo "reason=$1"
    echo "iterations=$2"
    echo "head=$(head_sha)"
    echo "verification=$( [ $ok -eq 0 ] && echo pass || echo fail )"
    echo "tasks_unchecked=$(remaining)"
    echo "blocked_tasks=$(blocked)"
  } > .driver_done
  return 0
}

write_done() {
  {
    echo "finished_at=$(date '+%Y-%m-%d %H:%M:%S')"
    echo "reason=$1"
    echo "iterations=$2"
    echo "head=$(head_sha)"
    echo "tasks_unchecked=$(remaining)"
    echo "blocked_tasks=$(blocked)"
  } > .driver_done
  return 0
}

# ------------------------------------------------- post-commit verification

# A commit is only allowed to stand if the tests pass after it. This is the
# defence against the agent ticking a box for work it did not actually do.
verify_after_commit() {
  local prev="$1" new="$2" task="$3" rc vlog
  [ "$VERIFY_AFTER_COMMIT" = "1" ] || return 0
  vlog="$RUNDIR/verify-$new.log"
  log "verifying commit $new with ./scripts/test.sh ..."
  ./scripts/test.sh > "$vlog" 2>&1; rc=$?
  if [ $rc -eq 0 ]; then
    log "  verification PASSED - keeping $new"
    return 0
  fi
  log "  verification FAILED (./scripts/test.sh exit $rc) - rolling back to $prev"
  tail -n 20 "$vlog" | logf
  git reset --hard "$prev" >/dev/null 2>&1
  {
    echo ""
    echo "$(date '+%Y-%m-%d %H:%M'): DRIVER REJECTED the commit for $task."
    echo "./scripts/test.sh exited $rc immediately after it, so the commit was rolled"
    echo "back. Tail of the failing output:"
    tail -n 15 "$vlog" | sed 's/^/    /'
    echo "Next run: ./scripts/test.sh must exit 0 before you commit. Exit 2 means the"
    echo "suite registered no real tests - add Tests/Test<Component>.cpp with a"
    echo "juce::UnitTest subclass and a static instance of it."
  } >> STATE.md
  git add STATE.md >/dev/null 2>&1
  git commit -q -m "harness: reject $task commit (scripts/test.sh exit $rc)" >/dev/null 2>&1 || true
  return 1
}

save_preflight_result() {
  {
    echo "STREAM_ARGS=\"$STREAM_ARGS\""
    echo "IDLE_TIMEOUT=$IDLE_TIMEOUT"
    echo "HAVE_FALLBACK=$HAVE_FALLBACK"
  } > "$RUNDIR/.preflight.env"
}

# ---------------------------------------------------------------------- main

trap 'log "driver received a signal - shutting down"; ollama_unload_all; rm -f .driver.pid; exit 130' INT TERM

if [ "$CHECK_ONLY" = "1" ]; then
  preflight; rc=$?
  save_preflight_result
  exit $rc
fi

echo $$ > .driver.pid
rm -f .driver_done
cp -f .env.motionengine "$RUNDIR/env.backup" 2>/dev/null || true

log "########## driver start: max_iter=$MAX_ITER model=$PRIMARY_MODEL wall_clock=${MAX_HOURS}h ##########"

if [ "$RUN_PREFLIGHT" = "1" ]; then
  preflight || die "preflight failed - fix the BAD lines above and re-run"
  save_preflight_result
elif [ -f "$RUNDIR/.preflight.env" ]; then
  # shellcheck disable=SC1091
  . "$RUNDIR/.preflight.env"
  log "preflight results carried over from start.sh (stream=${STREAM_ARGS:-off}, idle_timeout=${IDLE_TIMEOUT}s, fallback=$HAVE_FALLBACK)"
else
  log "preflight skipped and nothing cached - running without the idle watchdog"
  STREAM_ARGS=""; IDLE_TIMEOUT=0
fi

fails=0
i=0
prev_sha="$(head_sha)"
DEADLINE=$(( START_TS + MAX_HOURS * 3600 ))
STOP_REASON="iteration limit reached"

for i in $(seq 1 "$MAX_ITER"); do
  scrub_state
  restore_env_file

  left="$(remaining)"
  if [ "$left" -eq 0 ]; then
    STOP_REASON="all tasks resolved"
    final_verification "$STOP_REASON" "$i"
    break
  fi

  now="$(date +%s)"
  budget="$ITER_TIMEOUT"
  if [ "$MAX_HOURS" -gt 0 ]; then
    remain=$(( DEADLINE - now ))
    if [ "$remain" -le 600 ]; then
      log "less than 10 minutes left of the ${MAX_HOURS}h wall clock - stopping cleanly"
      STOP_REASON="wall-clock limit (${MAX_HOURS}h)"
      break
    fi
    [ "$remain" -lt "$budget" ] && budget="$remain"
  fi

  task="$(current_task)"; task="${task:-T-??}"
  tf="$(task_fails "$task")"

  memory_guard
  level_prepare "$LEVEL"
  m="$(level_model "$LEVEL")"
  ctx="$(level_ctx "$LEVEL")"

  log "=== iteration $i/$MAX_ITER | $task attempt $((tf + 1))/$MAX_TASK_FAILS | $left left, $(blocked) blocked | level $LEVEL | $m @ ${ctx}tok | budget $((budget / 60))m | HEAD $(head_sha) ==="

  ptext="$(cat PROMPT.md 2>/dev/null)"
  if [ -z "$ptext" ]; then
    log "PROMPT.md is empty - restoring it from git"
    git checkout -- PROMPT.md >/dev/null 2>&1
    ptext="$(cat PROMPT.md 2>/dev/null)"
    [ -n "$ptext" ] || die "PROMPT.md is empty and could not be restored"
  fi

  note=""
  if [ "$tf" -gt 0 ]; then
    note="$note
DRIVER NOTE: this is attempt $((tf + 1)) of $MAX_TASK_FAILS at $task. Earlier attempts
did not produce a commit that survives ./scripts/test.sh. Read the most recent
STATE.md entries before you start and do not repeat what already failed. After
$MAX_TASK_FAILS attempts the driver marks $task [BLOCKED] and moves on without you."
  fi
  if [ "$LEVEL" -ge 2 ]; then
    note="$note
DRIVER NOTE: you are running on a reduced ${FALLBACK_CTX}-token context. Read narrow
line ranges, never whole headers, and keep this run to the single current task."
  fi
  [ -n "$note" ] && ptext="$ptext

---$note"

  lgf="$RUNDIR/iter-$(printf '%03d' "$i").log"
  watched_run "$budget" "$IDLE_TIMEOUT" "$lgf" \
    env ANTHROPIC_MODEL="$m" \
        ANTHROPIC_DEFAULT_HAIKU_MODEL="$m" \
        ANTHROPIC_DEFAULT_SONNET_MODEL="$m" \
        ANTHROPIC_DEFAULT_OPUS_MODEL="$m" \
        ANTHROPIC_SMALL_FAST_MODEL="$m" \
        ANTHROPIC_CUSTOM_MODEL_OPTION="$m" \
        CLAUDE_CODE_MAX_CONTEXT_TOKENS="$ctx" \
        claude --model "$m" --dangerously-skip-permissions $STREAM_ARGS --print "$ptext"
  rc=$?
  digest "$lgf"

  new_sha="$(head_sha)"
  progressed=0

  if [ "$new_sha" != "$prev_sha" ]; then
    restore_harness "$prev_sha" "$new_sha"
    new_sha="$(head_sha)"
    if verify_after_commit "$prev_sha" "$new_sha" "$task"; then progressed=1; fi
  else
    log "no commit landed this iteration"
    reset_uncommitted_work
  fi

  if [ "$progressed" = "1" ]; then
    fails=0
    set_task_fails "$task" 0
    if [ "$LEVEL" -gt 0 ]; then
      LEVEL=$((LEVEL - 1))
      log "progress made - recovery level eased back to $LEVEL"
    fi
  else
    fails=$((fails + 1))
    tf=$(( $(task_fails "$task") + 1 ))
    set_task_fails "$task" "$tf"
    case "$rc" in
      124) log "iteration $i hit the ${budget}s hard timeout"; LEVEL=$((LEVEL + 2)) ;;
      125) log "iteration $i went silent for ${IDLE_TIMEOUT}s and was killed"; LEVEL=$((LEVEL + 2)) ;;
      0)   log "iteration $i exited cleanly but produced nothing that survived"; LEVEL=$((LEVEL + 1)) ;;
      *)   log "iteration $i exited $rc"; LEVEL=$((LEVEL + 1)) ;;
    esac
    [ "$LEVEL" -gt 3 ] && LEVEL=3
    log "consecutive failures $fails/$MAX_CONSEC_FAIL | $task attempts $tf/$MAX_TASK_FAILS | recovery level $LEVEL"
    if [ "$tf" -ge "$MAX_TASK_FAILS" ]; then
      block_task "$task" "last exit $rc at recovery level $LEVEL"
      LEVEL=0
      fails=0
    fi
  fi

  if [ "$fails" -ge "$MAX_CONSEC_FAIL" ]; then
    log "ABORT: $MAX_CONSEC_FAIL consecutive failures at every recovery level - something is genuinely broken."
    STOP_REASON="$MAX_CONSEC_FAIL consecutive failures"
    break
  fi

  prev_sha="$(head_sha)"
  sleep "$COOLDOWN"
done

log "########## driver stop: $STOP_REASON | $(remaining) unchecked, $(blocked) blocked | HEAD $(head_sha) ##########"
[ -f .driver_done ] || write_done "$STOP_REASON" "$i"
ollama_unload_all
log "model unloaded from memory"
command -v osascript >/dev/null 2>&1 && \
  osascript -e "display notification \"MotionEngine: $STOP_REASON\" with title \"drive.sh\"" >/dev/null 2>&1
rm -f .driver.pid
exit 0
