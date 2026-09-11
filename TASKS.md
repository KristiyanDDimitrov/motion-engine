# MotionEngine Task List

- [x] T-01 Bootstrap repo, CMake, scripts, empty test runner that builds and passes
- [x] T-02 EnvelopeFollower + tests: rises on burst, decays toward zero, never negative, longer attack reaches peak later
- [x] T-03 TransientDetector + tests: exactly one trigger per burst onset, none during sustain, respects refractory period
- [x] T-04 LFO + tests: measured period matches requested rate at 44.1k and 48k, output within [-1,1], phase offset shifts output, reset is deterministic
- [x] T-05 LFO tempo sync + tests: division-to-Hz conversion correct at 90/174/200 BPM
- [x] T-06 ModMatrix + tests: contributions sum, results clamp to destination range, depth 0 is a no-op, negative depth inverts
- [x] T-07 FilterStage + tests: rising cutoff raises spectral centroid monotonically, no NaN/Inf, stable at max resonance
- [x] T-08 DriveStage + tests: drive increases harmonic content vs clean, mix=0 is bit-identical, no NaN/Inf
- [x] T-09 Wire APVTS with every parameter, GenericAudioProcessorEditor, state save/restore round-trip test
- [x] T-10 Assemble full processBlock chain: analysis -> matrix -> filter -> drive -> gain/mix
- [ ] T-11 Robustness tests: 44.1/48/96 kHz, block sizes 32/64/512/2048, silence in gives silence out, no NaN anywhere
- [ ] T-12 pluginval strictness 5 passes
- [ ] T-13 pluginval strictness 10 passes; fix whatever it surfaces
- [ ] T-14 Write README.md: what it is, how to build, how to load in Live, current limitations
