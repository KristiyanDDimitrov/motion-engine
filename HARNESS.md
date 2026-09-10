# Running MotionEngine overnight

Two commands. Everything else is automatic.

```bash
cd ~/dev/motion-engine
./start.sh          # evening: preflight, then detach and run
./stop.sh           # morning: stop and free the model from RAM
./status.sh         # what happened overnight
```

`./start.sh` runs a full preflight first — build, tests, Ollama, one model round
trip — and **refuses to start** if anything is broken, while you are still at the
keyboard. Then it detaches under `caffeinate` and you can close the terminal.

**Leave the lid open.** macOS sleeps on lid close regardless of `caffeinate`
unless an external display is attached.

## Options

```bash
./start.sh 100              # iteration cap (default 60)
MAX_HOURS=8 ./start.sh      # wall clock (default 24h, 0 = unlimited)
./stop.sh --quit-ollama     # also quit the Ollama server, not just the model
./drive.sh --check          # preflight only, change nothing
./doctor.sh > logs/doctor.txt 2>&1   # full environment dump when something is off
```

## What runs unattended

Each iteration feeds `PROMPT.md` to a fresh `claude -p` against the local model.
The agent does one task from `TASKS.md`, commits, and exits. Context never
accumulates across iterations.

**Completion is decided by the driver, never by the agent.** It counts unchecked
boxes in `TASKS.md` and runs the tests itself. Nothing the model writes can start
or stop the loop.

**Every commit is verified.** After the agent commits, the driver runs
`./scripts/test.sh`. If it does not exit 0 the commit is rolled back and the
reason is appended to `STATE.md` for the next run to read. A ticked box with no
working code does not survive.

## When something fails

Failure never ends the night. The driver steps down a recovery ladder and steps
back up when progress resumes:

| Level | What it does |
|-------|--------------|
| 0 | Primary model, as configured |
| 1 | Unload the model first (clears memory pressure), retry |
| 2 | Reduced-context fallback model, built automatically from the primary Modelfile |
| 3 | Restart Ollama, then the fallback model |

A timeout or a silent stall jumps straight to level 2, since context pressure is
the usual cause. After three failed attempts at one task the driver marks it
`[BLOCKED]`, records why, and moves on to the next task.

Also running every iteration: a free-memory guard, a 40-minute idle watchdog, a
2-hour hard timeout, `STATE.md` trimming, completion-sentinel scrubbing, and
automatic repair of any harness file the agent edited.

The idle watchdog only works when the local endpoint streams its output, so
preflight probes for that explicitly and says which mode it picked. Watch for
`idle watchdog ACTIVE` in the preflight output — with it disabled, a stuck
iteration costs the full 2-hour timeout instead of 40 minutes. The probe runs
*after* the model is warm, because a cold load used to make it time out and
silently fall back.

## Memory is the binding constraint

The primary model is 18GB of weights plus a 64k KV cache, on a machine with
24GB of unified memory. macOS itself wants 4–6GB. There is no room left for
clang, and the 2026-09-09 run spent all nine hours at 800MB–1GB free with
4.5–13.6GB of swap. It survived the night and finished three tasks, but the
machine wedged the moment the display woke up.

What the harness now does about it:

- `scripts/build.sh` **unloads the model before compiling**, so the build gets
  the whole machine. Ollama reloads it on the next inference call (~90s).
- Build parallelism is chosen from free memory at build time, not core count.
- The watchdog samples memory every minute and **kills an iteration whose swap
  stays above 8GB** for three minutes, rather than letting it drag the machine
  down. `logs/memory.csv` is the trace to read in the morning.

  Result on 2026-09-10, with the build isolation in place: swap averaged
  4.7GB and peaked at 6.25GB, against 13.6GB the night before. The ceiling was
  never hit. Free memory still averaged only 1.1GB with a floor of 413MB, so
  the machine is out of danger but not out of swap. Note that the free-memory
  figures printed in `run.log` between iterations look healthy (~20GB) because
  they are sampled just after the model is unloaded — `logs/memory.csv` is the
  honest picture.
- JUCE's own internal unit tests are no longer compiled in. They were adding
  12.4 million assertions and an audio-reader fuzzer to every verification.

**The real fix is a smaller model.** Nothing above changes the arithmetic: an
18GB model on 24GB will always run this close to the edge. A 14B-class coder at
Q4 (~9GB) leaves room for the toolchain. Switch by editing `ANTHROPIC_MODEL` in
`.env.motionengine` — nothing else needs to change.

## Sleep and displays

**Leave the lid open**, even with an external monitor.

With the lid closed, the external display is the machine's *only* display, so
switching the monitor off at its own button leaves macOS headless. Waking it
then forces the whole display stack to page back in at once — and on a machine
already deep in swap, that is what produced the grey login screen and the
permanent beach ball.

Lid open, the internal display always exists. The external monitor can sleep or
be switched off freely, and the wake is cheap. It also vents better.

## Layout

| File | Role |
|------|------|
| `start.sh` `stop.sh` `status.sh` `doctor.sh` | what you run |
| `drive.sh` | the loop; decides completion, verifies commits, handles recovery |
| `PROMPT.md` | what the agent is told each iteration |
| `CLAUDE.md` | the DSP spec and hard constraints (auto-loaded by Claude Code) |
| `TASKS.md` | the authoritative task list |
| `STATE.md` | the only memory that survives between iterations |
| `.env.motionengine` | local model routing — never copy this into `~/.zshrc` |
| `run.log` | driver log · `logs/iter-NNN.log` — full agent output per iteration |

`drive.sh`, `start.sh`, `stop.sh`, `status.sh`, `doctor.sh`, `PROMPT.md`,
`CLAUDE.md`, `Tests/TestMain.cpp`, `scripts/*.sh` and `.env.motionengine` are
harness files. The agent's edits to them are reverted automatically.

## Adding tests

CMake globs `Source/*.cpp` and `Tests/*.cpp`, so `CMakeLists.txt` never needs
editing. A new test is a new file, `Tests/TestEnvelopeFollower.cpp`, holding a
`juce::UnitTest` subclass and one static instance of it.

`./scripts/test.sh` exit codes, which the driver acts on:
`0` passed · `1` an assertion failed · `2` no real tests registered ·
`3` build failed · `4` binary missing. Only `0` counts as done.
