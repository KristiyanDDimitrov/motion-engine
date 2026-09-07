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
MAX_HOURS=8 ./start.sh      # wall clock (default 10h, 0 = unlimited)
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
2h30 hard timeout, `STATE.md` trimming, completion-sentinel scrubbing, and
automatic repair of any harness file the agent edited.

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
