# MotionEngine — autonomous build prompt

You are working unattended in this repository. A driver loop re-invokes you
with this same prompt until every task is resolved. Do ONE task and exit.

`CLAUDE.md` is already loaded for you and holds the full DSP spec, the build
commands and the hard constraints. Do not re-read it unless you need a detail.
`TASKS.md` is the only authoritative task list.

## What to do this run

1. Open `TASKS.md`. Find the FIRST task marked `- [ ]` (skip `[x]` and
   `[BLOCKED]`). That is your task. Do only that one.
2. Read the last ~30 lines of `STATE.md` first. If this task has been tried
   before, the reason it failed is written there. Do not repeat it.
3. Implement the task. Confirm every JUCE symbol you use by grepping the real
   headers in `JUCE/modules/` — read narrow line ranges, never whole headers.
4. Run `./scripts/test.sh`. It must exit 0. Its exit codes are:
   - `0` everything passed
   - `1` an assertion failed — fix the code or the test
   - `2` the suite registered no real tests — you have not written any yet
   - `3` build or configure failed
   - `4` the test binary was not produced
5. Only when `./scripts/test.sh` exits 0: tick the box in `TASKS.md`, append
   one dated line to `STATE.md`, and commit everything with
   `[T-XX] short description`.
6. Exit.

## How tests work here

- Add tests as a NEW file: `Tests/TestEnvelopeFollower.cpp`, and so on.
  CMake globs `Tests/*.cpp` and `Source/*.cpp`, so **you never need to edit
  `CMakeLists.txt`**. If you believe you do, you are wrong — write why in
  `STATE.md` and carry on without doing it.
- A test file contains a `juce::UnitTest` subclass and one static instance:

  ```cpp
  #include <JuceHeader.h>
  #include "dsp/EnvelopeFollower.h"

  struct EnvelopeFollowerTests final : public juce::UnitTest
  {
      EnvelopeFollowerTests() : juce::UnitTest ("EnvelopeFollower", "dsp") {}
      void runTest() override
      {
          beginTest ("output is never negative");
          expect (value >= 0.0f, "envelope went negative");
      }
  };
  static EnvelopeFollowerTests envelopeFollowerTests;
  ```

- `Tests/TestMain.cpp` is the runner. Never edit it.
- Every named behaviour in the task line needs its own `beginTest` block. A
  task that says "longer attack reaches peak later" is not done until a test
  actually measures that.

## What the driver does that you cannot change

- After every commit, the driver runs `./scripts/test.sh` itself. If it does
  not exit 0, **your commit is rolled back** and the reason is appended to
  `STATE.md`. Ticking a box without working code gains you nothing.
- If the same task fails three runs, the driver marks it `[BLOCKED]` and moves
  on. You do not need to manage that.
- `drive.sh`, `start.sh`, `stop.sh`, `status.sh`, `doctor.sh`, `PROMPT.md`,
  `CLAUDE.md`, `Tests/TestMain.cpp`, `scripts/*.sh` and `.env.motionengine`
  are harness files. Edits to them are reverted automatically. They are never
  the fix for a task.
- Do not write completion phrases anywhere. Nothing you write starts or stops
  the loop; the driver counts checkboxes and runs the tests. Sentences like
  "all tasks complete" get stripped from `STATE.md` and waste a run.

## STATE.md

Append one dated entry per run: the task id, what you did, pass or fail, and
the single most useful thing the next run needs to know. Keep it factual and
short — it is the only memory that survives between runs. Never describe work
you did not actually do; the next run trusts this file.
