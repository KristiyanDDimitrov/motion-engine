# MotionEngine

Audio-effect VST3 plugin built with JUCE 8. Input is the user's already-designed bass patch (Serum, Vital, whatever). This plugin does not generate tone. It analyses the incoming audio and uses that analysis, plus its own modulators, to drive a processing chain. All analysis is audio-derived; there is no MIDI input in v1.

## Building

```bash
./scripts/build.sh            # configure + build Release
./scripts/build.sh --clean    # wipe build/ first
./scripts/test.sh             # build, then run MotionEngineTests
./scripts/validate.sh         # pluginval, strictness 5
./scripts/validate.sh 10      # pluginval, strictness 10
```

## Plugin Structure

The plugin processes audio through the following chain:
1. **Analysis** - EnvelopeFollower and TransientDetector analyze incoming audio
2. **Modulation** - LFOs and ModMatrix drive parameter changes based on analysis
3. **Processing** - FilterStage and DriveStage apply effects to the audio
4. **Output** - Final gain and mixing

## Components

### DSP Components
- `EnvelopeFollower` — rectify, then attack/release smoothing. Params: attack 0.1–200 ms, release 1–1000 ms, sensitivity 0–1. Output 0..1, never negative.
- `TransientDetector` — fast/slow envelope difference with a threshold and a refractory period. Emits a one-sample trigger on onset only, not during sustain.
- `LFO` — two instances. Shapes: sine, triangle, saw, square, random sample-and-hold, stepped. Rate either free (0.01–40 Hz) or tempo-synced to host divisions (1/1 down to 1/32, plus dotted and triplet). Params: rate, shape, phase offset, depth. Output bounded to [-1, 1]. Deterministic after `reset()`.
- `ModMatrix` — four slots. Each slot is {source, destination, depth -1..+1}. Sources: Envelope, Transient, LFO1, LFO2, Macro, Constant. Destinations: FilterCutoff, FilterResonance, DriveAmount, OutputGain, LFO2Rate. Contributions to the same destination sum, then clamp to that destination's legal range. Depth 0 must be a true no-op.
- `FilterStage` — state-variable filter, modes LP/BP/HP/notch. Cutoff 20–20000 Hz, resonance 0.1–20. Must stay stable at extreme resonance.
- `DriveStage` — waveshaper, types soft clip / hard clip / wavefold. Params: amount, type, mix. `mix = 0` must be bit-identical bypass.

## License

MIT