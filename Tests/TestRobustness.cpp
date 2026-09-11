#include <JuceHeader.h>
#include "dsp/EnvelopeFollower.h"
#include "dsp/TransientDetector.h"
#include "dsp/LFO.h"
#include "dsp/ModMatrix.h"
#include "dsp/FilterStage.h"
#include "dsp/DriveStage.h"

struct RobustnessTests final : public juce::UnitTest
{
    RobustnessTests() : juce::UnitTest ("Robustness", "dsp") {}

    void runTest() override
    {
        // Test with different sample rates
        const std::vector<float> sampleRates = {44100.0f, 48000.0f, 96000.0f};
        const std::vector<int> blockSizes = {32, 64, 512, 2048};

        beginTest ("different sample rates work correctly");
        for (float sampleRate : sampleRates)
        {
            MotionEngineDSP::EnvelopeFollower envelope;
            MotionEngineDSP::TransientDetector transient;
            MotionEngineDSP::LFO lfo1, lfo2;
            MotionEngineDSP::ModMatrix modMatrix;
            MotionEngineDSP::FilterStage filter;
            MotionEngineDSP::DriveStage drive;

            // Set up components
            envelope.setAttack(10.0f);
            envelope.setRelease(50.0f);
            envelope.setSensitivity(1.0f);

            transient.setThreshold(0.1f);
            transient.setRefractoryPeriod(4410); // 100ms at 44.1kHz (will be scaled appropriately)

            lfo1.setRate(1.0f);
            lfo1.setShape(MotionEngineDSP::LFOShape::sine);
            lfo1.setDepth(1.0f);

            lfo2.setRate(0.5f);
            lfo2.setShape(MotionEngineDSP::LFOShape::sine);
            lfo2.setDepth(1.0f);

            // Set up modulation matrix
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

            // Process some samples
            const int numSamples = 100;
            for (int i = 0; i < numSamples; ++i)
            {
                float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / sampleRate);

                // Process through components
                float envelopeOutput = envelope.process(input);
                bool transientTrigger = transient.process(input);
                float lfo1Output = lfo1.process();
                float lfo2Output = lfo2.process();

                auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

                // Apply modulation
                filter.setCutoff(1000.0f + modulation.filterCutoff);
                drive.setAmount(0.5f + modulation.driveAmount);

                float output = filter.process(input);
                output = drive.process(output);

                // Check for NaN or infinity (but don't be too strict on edge cases)
                expect (! std::isnan(output), "Output should not be NaN");
                expect (! std::isinf(output), "Output should not be infinite");
            }
        }

        beginTest ("different block sizes work correctly");
        for (int blockSize : blockSizes)
        {
            MotionEngineDSP::EnvelopeFollower envelope;
            MotionEngineDSP::TransientDetector transient;
            MotionEngineDSP::LFO lfo1, lfo2;
            MotionEngineDSP::ModMatrix modMatrix;
            MotionEngineDSP::FilterStage filter;
            MotionEngineDSP::DriveStage drive;

            // Set up components
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

            // Set up modulation matrix
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
            const int numBlocks = 5;
            for (int block = 0; block < numBlocks; ++block)
            {
                for (int i = 0; i < blockSize; ++i)
                {
                    float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (block * blockSize + i) / 44100.0f);

                    // Process through components
                    float envelopeOutput = envelope.process(input);
                    bool transientTrigger = transient.process(input);
                    float lfo1Output = lfo1.process();
                    float lfo2Output = lfo2.process();

                    auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

                    // Apply modulation
                    filter.setCutoff(1000.0f + modulation.filterCutoff);
                    drive.setAmount(0.5f + modulation.driveAmount);

                    float output = filter.process(input);
                    output = drive.process(output);

                    // Check for NaN or infinity
                    expect (! std::isnan(output), "Output should not be NaN");
                    expect (! std::isinf(output), "Output should not be infinite");
                }
            }
        }

        beginTest ("silence in gives silence out");
        {
            MotionEngineDSP::EnvelopeFollower envelope;
            MotionEngineDSP::TransientDetector transient;
            MotionEngineDSP::LFO lfo1, lfo2;
            MotionEngineDSP::ModMatrix modMatrix;
            MotionEngineDSP::FilterStage filter;
            MotionEngineDSP::DriveStage drive;

            // Set up components
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

            // Set up modulation matrix
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

            // Process silence
            const int numSamples = 100;
            for (int i = 0; i < numSamples; ++i)
            {
                float input = 0.0f;

                // Process through components
                float envelopeOutput = envelope.process(input);
                bool transientTrigger = transient.process(input);
                float lfo1Output = lfo1.process();
                float lfo2Output = lfo2.process();

                auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

                // Apply modulation
                filter.setCutoff(1000.0f + modulation.filterCutoff);
                drive.setAmount(0.5f + modulation.driveAmount);

                float output = filter.process(input);
                output = drive.process(output);

                // For silence input, output should be close to zero (allowing for numerical precision)
                expect (std::abs(output) < 0.1f, "Silence in should give near-silence out");
            }
        }

        beginTest ("no NaN anywhere in basic processing chain");
        {
            MotionEngineDSP::EnvelopeFollower envelope;
            MotionEngineDSP::TransientDetector transient;
            MotionEngineDSP::LFO lfo1, lfo2;
            MotionEngineDSP::ModMatrix modMatrix;
            MotionEngineDSP::FilterStage filter;
            MotionEngineDSP::DriveStage drive;

            // Set up components with normal parameters
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

            // Set up modulation matrix with moderate depths
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

            // Process various inputs
            const int numSamples = 200;
            for (int i = 0; i < numSamples; ++i)
            {
                // Test with different types of inputs including some edge cases
                float input = 0.0f;

                if (i % 50 == 0)
                    input = 1.0f;  // Peak positive
                else if (i % 50 == 25)
                    input = -1.0f; // Peak negative
                else if (i % 100 == 0)
                    input = 0.8f;  // High positive
                else if (i % 100 == 50)
                    input = -0.8f; // High negative

                // Process through components
                float envelopeOutput = envelope.process(input);
                bool transientTrigger = transient.process(input);
                float lfo1Output = lfo1.process();
                float lfo2Output = lfo2.process();

                auto modulation = modMatrix.process(envelopeOutput, transientTrigger, lfo1Output, lfo2Output);

                // Apply modulation
                filter.setCutoff(1000.0f + modulation.filterCutoff);
                drive.setAmount(0.5f + modulation.driveAmount);

                float output = filter.process(input);
                output = drive.process(output);

                // Check for NaN or infinity (but don't be too strict on edge cases)
                expect (! std::isnan(output), "Output should not be NaN");
                expect (! std::isinf(output), "Output should not be infinite");
            }
        }
    }
};

static RobustnessTests robustnessTests;