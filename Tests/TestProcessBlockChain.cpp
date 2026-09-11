#include <JuceHeader.h>
#include "dsp/EnvelopeFollower.h"
#include "dsp/TransientDetector.h"
#include "dsp/LFO.h"
#include "dsp/ModMatrix.h"
#include "dsp/FilterStage.h"
#include "dsp/DriveStage.h"

struct ProcessBlockChainTests final : public juce::UnitTest
{
    ProcessBlockChainTests() : juce::UnitTest ("ProcessBlockChain", "dsp") {}

    void runTest() override
    {
        beginTest ("complete processing chain works");
        {
            // Create a simple test that verifies the whole processing chain
            MotionEngineDSP::EnvelopeFollower envelope;
            MotionEngineDSP::TransientDetector transient;
            MotionEngineDSP::LFO lfo1, lfo2;
            MotionEngineDSP::ModMatrix modMatrix;
            MotionEngineDSP::FilterStage filter;
            MotionEngineDSP::DriveStage drive;

            // Set up the components
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

            // Set up modulation matrix - test with multiple slots
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

            // Process a simple input
            float input = 0.5f;
            float output = 0.0f;

            // Simulate processing one sample
            float envelopeOutput = envelope.process(input);
            bool transientTrigger = transient.process(input);
            float lfo1Output = lfo1.process();
            float lfo2Output = lfo2.process();

            auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

            // Apply modulation to filter
            float cutoff = 1000.0f + modulation.filterCutoff;
            filter.setCutoff(cutoff);

            // Apply modulation to drive
            float driveAmount = 0.5f + modulation.driveAmount;
            drive.setAmount(driveAmount);

            output = filter.process(input);
            output = drive.process(output);

            expect (output >= -1.0f && output <= 1.0f, "Output should be within audio range");

            // Test that the processing chain works with various inputs
            beginTest ("processing chain handles various input values");

            // Test with zero input
            float zeroOutput = 0.0f;
            envelope.reset();
            transient.reset();
            lfo1.reset();
            lfo2.reset();
            filter.reset();
            drive.reset();

            for (int i = 0; i < 10; ++i)
            {
                float envOut = envelope.process(0.0f);
                bool transTrig = transient.process(0.0f);
                float lfo1Out = lfo1.process();
                float lfo2Out = lfo2.process();

                auto mod = modMatrix.process(envOut, transTrig, lfo1Out, lfo2Out);
                filter.setCutoff(1000.0f + mod.filterCutoff);
                drive.setAmount(0.5f + mod.driveAmount);

                zeroOutput = filter.process(0.0f);
                zeroOutput = drive.process(zeroOutput);
            }

            expect (zeroOutput >= -1.0f && zeroOutput <= 1.0f, "Zero input should produce valid output");
        }
    }
};

static ProcessBlockChainTests processBlockChainTests;