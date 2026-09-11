#include <JuceHeader.h>
#include "dsp/EnvelopeFollower.h"
#include "dsp/TransientDetector.h"
#include "dsp/LFO.h"
#include "dsp/ModMatrix.h"
#include "dsp/FilterStage.h"
#include "dsp/DriveStage.h"

struct CompleteChainTests final : public juce::UnitTest
{
    CompleteChainTests() : juce::UnitTest ("CompleteChain", "dsp") {}

    void runTest() override
    {
        beginTest ("full processing chain produces valid output");

        // Create all components of the processing chain
        MotionEngineDSP::EnvelopeFollower envelope;
        MotionEngineDSP::TransientDetector transient;
        MotionEngineDSP::LFO lfo1, lfo2;
        MotionEngineDSP::ModMatrix modMatrix;
        MotionEngineDSP::FilterStage filter;
        MotionEngineDSP::DriveStage drive;

        // Set up parameters
        envelope.setAttack(10.0f);
        envelope.setRelease(50.0f);
        envelope.setSensitivity(1.0f);

        transient.setThreshold(0.1f);
        transient.setRefractoryPeriod(4410); // 100ms at 44.1kHz

        lfo1.setRate(1.0f);
        lfo1.setShape(MotionEngineDSP::LFOShape::sine);
        lfo1.setDepth(1.0f);

        lfo2.setRate(0.5f);
        lfo2.setShape(MotionEngineDSP::LFOShape::sine);
        lfo2.setDepth(1.0f);

        // Set up modulation matrix - use all sources
        modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::envelope,
                          MotionEngineDSP::ModMatrixDestination::filterCutoff, 500.0f);
        modMatrix.setSlot(1, MotionEngineDSP::ModMatrixSource::lfo1,
                          MotionEngineDSP::ModMatrixDestination::driveAmount, 0.5f);
        modMatrix.setSlot(2, MotionEngineDSP::ModMatrixSource::lfo2,
                          MotionEngineDSP::ModMatrixDestination::filterResonance, 2.0f);

        filter.setMode(MotionEngineDSP::FilterMode::lowpass);
        filter.setCutoff(1000.0f);
        filter.setResonance(1.0f);

        drive.setAmount(0.5f);
        drive.setType(MotionEngineDSP::DriveType::softClip);
        drive.setMix(0.5f);

        // Process a block of samples
        const int numSamples = 100;
        float input = 0.5f;
        float output = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Process each component in sequence
            float envelopeOutput = envelope.process(input);
            bool transientTrigger = transient.process(input);
            float lfo1Output = lfo1.process();
            float lfo2Output = lfo2.process();

            // Get modulation from matrix
            auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

            // Apply modulation to filter parameters
            float cutoff = 1000.0f + modulation.filterCutoff;
            float resonance = 1.0f + modulation.filterResonance;

            filter.setCutoff(cutoff);
            filter.setResonance(resonance);

            // Apply modulation to drive
            float driveAmount = 0.5f + modulation.driveAmount;
            drive.setAmount(driveAmount);

            // Process through filter and drive stages
            output = filter.process(input);
            output = drive.process(output);

            // Verify that output is within valid range
            expect (output >= -1.0f && output <= 1.0f, "Output should be within audio range");
        }
    }
};

static CompleteChainTests completeChainTests;