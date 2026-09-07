# MotionEngine

Audio-effect VST3 plugin built with JUCE 8. Input is the user's already-designed
bass patch (Serum, Vital, whatever). This plugin does not generate tone. It
analyses the incoming audio and uses that analysis, plus its own modulators, to
drive a processing chain. All analysis is audio-derived; there is no MIDI input
in v1.

## Commands

```bash
./scripts/build.sh            # configure + build Release
./scripts/build.sh --clean    # wipe build/ first
./scripts/test.sh             # build, then run MotionEngineTests
./scripts/validate.sh         # pluginval, strictness 5
./scripts/validate.sh 10      # pluginval, strictness 10
```

`./scripts/test.sh` exit codes — the driver acts on these, so read them:
`0` all passed · `1` an assertion failed · `2` no real tests registered ·
`3` build failed · `4` test binary missing. Only `0` counts as done.

Build artefacts:
- plugin: `build/MotionEngine_artefacts/Release/VST3/MotionEngine.vst3`
- tests:  `build/MotionEngineTests_artefacts/Release/MotionEngineTests`

## Tests

- Add a test as a NEW file `Tests/Test<Component>.cpp` holding a
  `juce::UnitTest` subclass plus one static instance of it. It is discovered
  automatically at runtime and compiled automatically by CMake.
- `CMakeLists.txt` globs `Source/*.cpp` and `Tests/*.cpp`. **You never need to
  edit it.** No target_sources edits, ever.
- `Tests/TestMain.cpp` is the runner and is off limits.
- The runner exits nonzero if any assertion fails, and also if the only thing
  registered is its own self test — an empty suite is not a pass.
- Every named behaviour in a task line gets its own `beginTest` block.

## Hard constraints

- Never modify anything inside `JUCE/`. It is a pinned submodule.
- Never modify `drive.sh`, `start.sh`, `stop.sh`, `status.sh`, `doctor.sh`,
  `scripts/*.sh`, `Tests/TestMain.cpp` or `.env.motionengine`. These run the
  loop that invokes you; they are not part of the plugin project and are never
  the right fix for a task. Edits to them are reverted automatically. If a task
  seems to require changing one, it doesn't — write why in `STATE.md` instead.
- Never modify `PROMPT.md` or this file (`CLAUDE.md`). Your instructions and
  the spec are not something to edit mid-task.
- Never write a completion phrase anywhere. The driver decides completion from
  checkbox counts and test exit codes; such phrases are stripped and the run is
  wasted.
- Never run `git push`. Never `rm -rf` outside this repo. Never write to
  `~/Library` except via CMake's `COPY_PLUGIN_AFTER_BUILD`.
- Do not recall JUCE APIs from memory. `grep -rn` the actual headers in
  `JUCE/modules/` to confirm class and method names before using them. If a
  symbol is not in the headers, it does not exist. Read only the matching
  region, never the whole header.
- Everything in `Source/dsp/` is header-only. No `.cpp` files there.
- No custom GUI. Use `juce::GenericAudioProcessorEditor` so parameters are
  usable in Live immediately and GUI tests pass trivially.
- No allocation, locking, or logging inside `processBlock`.
- Tests use `juce::UnitTest` / `UnitTestRunner` only. No other test framework.
- Commit message format: `[T-XX] short description`.

## DSP spec

**Analysis**
- `EnvelopeFollower` — rectify, then attack/release smoothing. Params: attack
  0.1–200 ms, release 1–1000 ms, sensitivity 0–1. Output 0..1, never negative.
- `TransientDetector` — fast/slow envelope difference with a threshold and a
  refractory period. Emits a one-sample trigger on onset only, not during
  sustain.

**Modulators**
- `LFO` — two instances. Shapes: sine, triangle, saw, square, random sample-and-
  hold, stepped. Rate either free (0.01–40 Hz) or tempo-synced to host divisions
  (1/1 down to 1/32, plus dotted and triplet). Params: rate, shape, phase
  offset, depth. Output bounded to [-1, 1]. Deterministic after `reset()`.

**Routing**
- `ModMatrix` — four slots. Each slot is {source, destination, depth -1..+1}.
  Sources: Envelope, Transient, LFO1, LFO2, Macro, Constant. Destinations:
  FilterCutoff, FilterResonance, DriveAmount, OutputGain, LFO2Rate.
  Contributions to the same destination sum, then clamp to that destination's
  legal range. Depth 0 must be a true no-op.

**Processing chain**
- `FilterStage` — state-variable filter, modes LP/BP/HP/notch. Cutoff
  20–20000 Hz, resonance 0.1–20. Must stay stable at extreme resonance.
- `DriveStage` — waveshaper, types soft clip / hard clip / wavefold. Params:
  amount, type, mix. `mix = 0` must be bit-identical bypass.
- Output gain and a global dry/wet.

All parameters live in an `AudioProcessorValueTreeState` and must round-trip
through `getStateInformation` / `setStateInformation`.
