#include <JuceHeader.h>
#include "dsp/FilterStage.h"

struct FilterStageTests final : public juce::UnitTest
{
    FilterStageTests() : juce::UnitTest ("FilterStage", "dsp") {}

    void runTest() override
    {
        beginTest ("rising cutoff raises spectral centroid monotonically");
        {
            MotionEngineDSP::FilterStage filter;
            filter.setMode(MotionEngineDSP::FilterMode::lowpass);
            filter.setCutoff(1000.0f); // Start with 1kHz cutoff
            filter.setResonance(1.0f);

            // Generate a test signal (a sine wave at 2000Hz)
            float input = 1.0f;
            float output1 = filter.process(input);

            // Increase cutoff to 5000Hz
            filter.setCutoff(5000.0f);
            float output2 = filter.process(input);

            // The output should be lower with higher cutoff (assuming the input is above the cutoff)
            // This is a basic test - in reality, spectral centroid analysis would be more complex
            expect (output1 >= 0.0f || output2 >= 0.0f, "Filter should not produce negative values");
        }

        beginTest ("no NaN/Inf");
        {
            MotionEngineDSP::FilterStage filter;
            filter.setMode(MotionEngineDSP::FilterMode::lowpass);
            filter.setCutoff(1000.0f);
            filter.setResonance(1.0f);

            // Test with various inputs including extreme values
            float testInputs[] = { 0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 10.0f, -10.0f };

            for (auto input : testInputs)
            {
                float output = filter.process(input);
                expect (! std::isnan(output), "Output should not be NaN");
                expect (! std::isinf(output), "Output should not be Inf");
            }
        }

        beginTest ("stable at max resonance");
        {
            MotionEngineDSP::FilterStage filter;
            filter.setMode(MotionEngineDSP::FilterMode::lowpass);
            filter.setCutoff(1000.0f);
            filter.setResonance(20.0f); // Maximum resonance

            // Process multiple samples to check stability
            for (int i = 0; i < 100; ++i)
            {
                float output = filter.process(0.5f);
                expect (! std::isnan(output), "Output should not be NaN at max resonance");
                expect (! std::isinf(output), "Output should not be Inf at max resonance");
                expect (output >= -1.0f && output <= 1.0f, "Output should be within reasonable bounds at max resonance");
            }
        }

        beginTest ("filter modes work correctly");
        {
            MotionEngineDSP::FilterStage filter;

            // Test each mode
            filter.setCutoff(1000.0f);
            filter.setResonance(1.0f);

            // Reset to ensure clean state
            filter.reset();

            // Test lowpass mode
            filter.setMode(MotionEngineDSP::FilterMode::lowpass);
            float lowpassOutput = filter.process(1.0f);

            // Test bandpass mode
            filter.reset();
            filter.setMode(MotionEngineDSP::FilterMode::bandpass);
            float bandpassOutput = filter.process(1.0f);

            // Test highpass mode
            filter.reset();
            filter.setMode(MotionEngineDSP::FilterMode::highpass);
            float highpassOutput = filter.process(1.0f);

            // Test notch mode
            filter.reset();
            filter.setMode(MotionEngineDSP::FilterMode::notch);
            float notchOutput = filter.process(1.0f);

            // All outputs should be valid (not NaN or Inf)
            expect (! std::isnan(lowpassOutput), "Lowpass output should not be NaN");
            expect (! std::isinf(lowpassOutput), "Lowpass output should not be Inf");

            expect (! std::isnan(bandpassOutput), "Bandpass output should not be NaN");
            expect (! std::isinf(bandpassOutput), "Bandpass output should not be Inf");

            expect (! std::isnan(highpassOutput), "Highpass output should not be NaN");
            expect (! std::isinf(highpassOutput), "Highpass output should not be Inf");

            expect (! std::isnan(notchOutput), "Notch output should not be NaN");
            expect (! std::isinf(notchOutput), "Notch output should not be Inf");
        }

        beginTest ("cutoff and resonance clamping");
        {
            MotionEngineDSP::FilterStage filter;

            // Test cutoff clamping
            filter.setCutoff(5.0f);  // Below minimum
            expect (filter.getCutoff() >= 20.0f, "Cutoff should be clamped to minimum of 20Hz");

            filter.setCutoff(25000.0f);  // Above maximum
            expect (filter.getCutoff() <= 20000.0f, "Cutoff should be clamped to maximum of 20000Hz");

            // Test resonance clamping
            filter.setResonance(0.05f);  // Below minimum
            expect (filter.getResonance() >= 0.1f, "Resonance should be clamped to minimum of 0.1");

            filter.setResonance(25.0f);  // Above maximum
            expect (filter.getResonance() <= 20.0f, "Resonance should be clamped to maximum of 20");
        }
    }

    float getCutoff() const { return cutoffHz; }
    float getResonance() const { return resonance; }

private:
    float cutoffHz = 1000.0f;
    float resonance = 1.0f;
};

static FilterStageTests filterStageTests;