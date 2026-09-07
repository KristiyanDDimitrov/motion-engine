#!/usr/bin/env bash
# Environment diagnostic dump.  HARNESS FILE - DO NOT EDIT.
# Run this and share the output when something needs debugging:
#     ./doctor.sh > logs/doctor.txt 2>&1
set -uo pipefail
cd "$(dirname "$0")"

hr() { printf '\n===== %s =====\n' "$1"; }

hr "when / where"
date
echo "pwd: $PWD"
echo "shell: $BASH_VERSION"

hr "hardware + memory"
sysctl -n machdep.cpu.brand_string 2>/dev/null
echo "cpus: $(sysctl -n hw.ncpu 2>/dev/null)"
echo "ram : $(( $(sysctl -n hw.memsize 2>/dev/null || echo 0) / 1073741824 )) GB"
echo "page: $(sysctl -n hw.pagesize 2>/dev/null)"
sysctl -n vm.swapusage 2>/dev/null
vm_stat 2>/dev/null | head -6

hr "toolchain"
for c in claude cmake git curl ollama perl python3 pluginval gtimeout; do
  if command -v "$c" >/dev/null 2>&1; then printf '  %-10s %s\n' "$c" "$(command -v "$c")"
  else printf '  %-10s MISSING\n' "$c"; fi
done
[ -x /Applications/pluginval.app/Contents/MacOS/pluginval ] && echo "  pluginval.app present"
echo "  claude version: $(claude --version 2>&1 | head -n 1)"
echo "  cmake  version: $(cmake --version 2>&1 | head -n 1)"

hr "claude CLI flags we rely on"
claude --help 2>&1 | grep -E -- '--print|--output-format|--dangerously-skip-permissions|--model|--max-turns' | sed 's/^/  /'

hr "ollama"
curl -fsS --max-time 10 http://localhost:11434/api/tags >/dev/null 2>&1 \
  && echo "  server: UP" || echo "  server: DOWN"
ollama list 2>&1 | sed 's/^/  /'
echo "  --- resident now ---"
ollama ps 2>&1 | sed 's/^/  /'
echo "  --- primary model config ---"
MODEL="$(awk -F= '/^export ANTHROPIC_MODEL=/{print $2}' .env.motionengine 2>/dev/null | tr -d '"'"'"' ' | head -n 1)"
echo "  model from .env: ${MODEL:-<unset>}"
[ -n "${MODEL:-}" ] && ollama show "$MODEL" 2>&1 | sed 's/^/  /' | head -30
[ -n "${MODEL:-}" ] && echo "  --- modelfile ---" && ollama show "$MODEL" --modelfile 2>&1 | grep -vE '^(TEMPLATE|SYSTEM)' | head -25 | sed 's/^/  /'

hr "environment file"
cat .env.motionengine 2>/dev/null | sed 's/^/  /'

hr "git"
git --no-pager log --oneline -10 2>&1 | sed 's/^/  /'
echo "  status:"; git status --porcelain 2>&1 | sed 's/^/    /'
echo "  submodule:"; git submodule status 2>&1 | sed 's/^/    /'
echo "  identity: $(git config user.name 2>/dev/null) <$(git config user.email 2>/dev/null)>"

hr "tasks"
grep -E '^[[:space:]]*-[[:space:]]*\[' TASKS.md 2>/dev/null | sed 's/^/  /'

hr "build + test (this is the real gate)"
./scripts/test.sh 2>&1 | tail -40 | sed 's/^/  /'
echo "  scripts/test.sh exit code: ${PIPESTATUS[0]}"

hr "done"
echo "Share this whole output when asking for help."
