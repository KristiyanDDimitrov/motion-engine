# MotionEngine — autonomous build prompt

You are working unattended in this repository. Read this whole file, then do
exactly ONE unit of work and exit. A driver loop will re-invoke you with this
same prompt until all tasks are complete. Do not try to do everything in one
run.

## Operating rules

1. If `TASKS.md` does not exist, your ONLY job this run is BOOTSTRAP (below).
   Create the files, commit, update `STATE.md`, and exit.
2. If `TASKS.md` exists, open it, find the FIRST task not marked `[x]` or
   `[BLOCKED]`, and do only that task. Then commit, update `STATE.md`, exit.
3. Before every commit, run `./scripts/test.sh`. If it fails, fix it. Do not
   commit failing code.
4. If the same task fails three separate runs (check `STATE.md` history), mark
   it `[BLOCKED]` in `TASKS.md`, write why in `STATE.md`, and move on.
5. Append one dated line to `STATE.md` every run: iteration outcome, task id,
   pass/fail, and anything the next run needs to know. Keep `STATE.md` under
   150 lines — condense old entries rather than letting it grow.
6. When every task is `[x]` or `[BLOCKED]`, append the exact line
   `ALL TASKS COMPLETE` to `STATE.md`. The driver loop watches for this.

## Hard constraints

- Never modify anything inside `JUCE/`. It is a pinned dependency.
- Never run `git push`, never `rm -rf` outside this repo, never write to
  `~/Library` except via CMake's `COPY_PLUGIN_AFTER_BUILD`.
- Do not recall JUCE APIs from memory. `grep` the actual headers in
  `JUCE/modules/` to confirm class and method names before using them. If a
  symbol is not in the headers, it does not exist.
- No custom GUI. Use `juce::GenericAudioProcessorEditor` so parameters are
  usable in Live immediately and GUI tests pass trivially.
- No allocation, locking, or logging inside `processBlock`.
- Commit message format: `[T-XX] short description`.

## BOOTSTRAP (first run only)

Create this structure:

```
CMakeLists.txt
JUCE/                     (git submodule, JUCE 8 branch, pinned)
Source/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/EnvelopeFollower.h
  dsp/TransientDetector.h
  dsp/LFO.h
  dsp/ModMatrix.h
  dsp/FilterStage.h
  dsp/DriveStage.h
Tests/
  TestMain.cpp
scripts/
  build.sh
  test.sh
  validate.sh
CLAUDE.md
TASKS.md
STATE.md
```

- Add JUCE as a submodule on the JUCE 8 branch. pluginval's CMake path is only
  tested against JUCE 8.
- `CMakeLists.txt` defines two targets: the VST3 plugin `MotionEngine`
  (`IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `COPY_PLUGIN_AFTER_BUILD TRUE`)
  and a console executable `MotionEngineTests` built from `Tests/TestMain.cpp`
  plus the DSP headers.
- Tests use JUCE's built-in `juce::UnitTest` / `UnitTestRunner`. Do not add
  Catch2, GoogleTest, Python, or any other dependency. `MotionEngineTests` must
  return a nonzero exit code if any test fails.
- `scripts/build.sh` configures and builds Release.
- `scripts/test.sh` builds then runs `MotionEngineTests`, propagating exit code.
- `scripts/validate.sh` runs
  `pluginval --strictness-level 5 --skip-gui-tests --validate <path to built .vst3>`
  and propagates the exit code.
- `CLAUDE.md` records: build command, test command, validate command, the hard
  constraints above, and the DSP spec below. Keep it short and factual.
- `TASKS.md` contains the task list below as unchecked checkboxes.

## DSP spec

Audio-effect plugin. Input is the user's already-designed bass patch (Serum,
Vital, whatever). This plugin does not generate tone. It analyses the incoming
audio and uses that analysis, plus its own modulators, to drive a processing
chain. All analysis is audio-derived; there is no MIDI input in v1.

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

## Task list

- [ ] T-01 Bootstrap repo, CMake, scripts, empty test runner that builds and passes
- [ ] T-02 EnvelopeFollower + tests: rises on burst, decays toward zero, never negative, longer attack reaches peak later
- [ ] T-03 TransientDetector + tests: exactly one trigger per burst onset, none during sustain, respects refractory period
- [ ] T-04 LFO + tests: measured period matches requested rate at 44.1k and 48k, output within [-1,1], phase offset shifts output, reset is deterministic
- [ ] T-05 LFO tempo sync + tests: division-to-Hz conversion correct at 90/174/200 BPM
- [ ] T-06 ModMatrix + tests: contributions sum, results clamp to destination range, depth 0 is a no-op, negative depth inverts
- [ ] T-07 FilterStage + tests: rising cutoff raises spectral centroid monotonically, no NaN/Inf, stable at max resonance
- [ ] T-08 DriveStage + tests: drive increases harmonic content vs clean, mix=0 is bit-identical, no NaN/Inf
- [ ] T-09 Wire APVTS with every parameter, GenericAudioProcessorEditor, state save/restore round-trip test
- [ ] T-10 Assemble full processBlock chain: analysis -> matrix -> filter -> drive -> gain/mix
- [ ] T-11 Robustness tests: 44.1/48/96 kHz, block sizes 32/64/512/2048, silence in gives silence out, no NaN anywhere
- [ ] T-12 pluginval strictness 5 passes
- [ ] T-13 pluginval strictness 10 passes; fix whatever it surfaces
- [ ] T-14 Write README.md: what it is, how to build, how to load in Live, current limitations
