# MotionEngine

## Build
```bash
./scripts/build.sh
```

## Test
```bash
./scripts/test.sh
```

## Validate
```bash
./scripts/validate.sh
```

## Hard constraints
- Never modify anything inside `JUCE/`. It is a pinned dependency.
- Never run `git push`, never `rm -rf` outside this repo, never write to `~/Library` except via CMake's `COPY_PLUGIN_AFTER_BUILD`.
- No custom GUI. Use `juce::GenericAudioProcessorEditor` so parameters are usable in Live immediately and GUI tests pass trivially.
- No allocation, locking, or logging inside `processBlock`.
- Commit message format: `[T-XX] short description`.

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