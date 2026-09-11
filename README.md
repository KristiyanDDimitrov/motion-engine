# MotionEngine

Audio-effect VST3 plugin built with JUCE 8. Input is the user's already-designed bass patch (Serum, Vital, whatever). This plugin does not generate tone. It analyses the incoming audio and uses that analysis, plus its own modulators, to drive a processing chain. All analysis is audio-derived; there is no MIDI input in v1.

## Features

- **Audio Analysis**: 
  - Envelope follower for dynamic control
  - Transient detector for onset detection

- **Modulation**:
  - Two LFOs with multiple wave shapes (sine, triangle, saw, square, random sample-and-hold, stepped)
  - Tempo-synced rate support
  - Modulation matrix routing (4 slots)

- **Processing Chain**:
  - State-variable filter with LP/BP/HP/notch modes
  - Drive stage with waveshaping (soft clip, hard clip, wavefold)
  - Output gain and dry/wet mixing

## Building

### Prerequisites
- CMake 3.22+
- JUCE 8
- Xcode command line tools (macOS)

### Build Steps
```bash
./scripts/build.sh            # configure + build Release
./scripts/test.sh             # build, then run MotionEngineTests
./scripts/validate.sh         # pluginval, strictness 5
./scripts/validate.sh 10      # pluginval, strictness 10
```

## Loading in Digital Audio Workstations

The built VST3 plugin can be loaded directly into any DAW that supports VST3 plugins:
- Ableton Live
- Logic Pro X
- FL Studio
- Reaper
- Bitwig Studio
- And many others...

The plugin appears as "MotionEngine" and requires stereo input/output.

## Current Limitations

1. No MIDI input in v1 (analysis only)
2. All parameters are exposed through the standard JUCE parameter system
3. Sample rate handling is limited to what's supported by the underlying JUCE audio processing framework
4. No built-in presets or program management

## License

MIT License - see LICENSE file for details.