#!/usr/bin/env bash
cd "$(dirname "$0")"
source ./.env.motionengine
MAX_ITER=${1:-60}
for i in $(seq 1 "$MAX_ITER"); do
  echo "=== iteration $i  $(date) ===" | tee -a run.log
  claude -p "$(cat PROMPT.md)" \
    --dangerously-skip-permissions \
    --model qwen3-coder-cc-64k >> run.log 2>&1 || echo "iteration $i exited nonzero" >> run.log
  if [ -f STATE.md ] && grep -q "ALL TASKS COMPLETE" STATE.md; then
    echo "All tasks complete at iteration $i" | tee -a run.log
    break
  fi
  sleep 5
done
