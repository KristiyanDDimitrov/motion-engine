# MotionEngine

An audio effect plugin that analyzes incoming audio and uses that analysis, plus its own modulators, to drive a processing chain.

## Build Instructions

```bash
./scripts/build.sh
```

## Test Instructions

```bash
./scripts/test.sh
```

## Validate Instructions

```bash
./scripts/validate.sh
```

## Features

- Audio-derived analysis (EnvelopeFollower, TransientDetector)
- Modulators (LFO with multiple waveforms)
- Routing matrix (ModMatrix)
- Processing chain (FilterStage, DriveStage)
- Full parameter persistence via APVTS

## Architecture

The plugin is built using JUCE 8 and follows the standard VST3 plugin architecture.

## DSP Components

1. **EnvelopeFollower** - Rectify and smooth audio envelope
2. **TransientDetector** - Detect audio transients with refractory period
3. **LFO** - Low-frequency oscillator with multiple waveforms and tempo sync
4. **ModMatrix** - Routing matrix for modulating parameters
5. **FilterStage** - State-variable filter (LP/BP/HP/notch)
6. **DriveStage** - Waveshaper with soft/hard clip and wavefold options

## Plugin Parameters

All parameters are stored in AudioProcessorValueTreeState and support round-trip serialization.