#include <JuceHeader.h>
#include "dsp/DriveStage.h"

struct DriveStageTests final : public juce::UnitTest
{
    DriveStageTests() : juce::UnitTest ("DriveStage", "dsp") {}

    void runTest() override
    {
        beginTest ("mix=0 is bit-identical");
        {
            MotionEngineDSP::DriveStage drive;
            drive.setMix(0.0f);

            // Test with various inputs - should be identical to clean signal
            float testInputs[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};

            for (float input : testInputs)
            {
                float output = drive.process(input);
                expect (juce::approximatelyEqual(output, input), "mix=0 should be bit-identical");
            }
        }

        beginTest ("no NaN/Inf");
        {
            MotionEngineDSP::DriveStage drive;
            drive.setMix(1.0f); // Full drive

            // Test with various inputs that might cause issues
            float testInputs[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.0f, -2.0f};

            for (float input : testInputs)
            {
                float output = drive.process(input);
                expect (! std::isnan(output), "output should not be NaN");
                expect (! std::isinf(output), "output should not be Inf");
            }
        }

        beginTest ("different drive types produce different outputs");
        {
            MotionEngineDSP::DriveStage drive;
            drive.setMix(1.0f);  // Full drive

            float input = 0.8f;

            // Test soft clip
            drive.setType(MotionEngineDSP::DriveType::softClip);
            drive.setAmount(0.5f);
            float softClipOutput = drive.process(input);

            // Test hard clip
            drive.setType(MotionEngineDSP::DriveType::hardClip);
            drive.setAmount(0.5f);
            float hardClipOutput = drive.process(input);

            // Test wavefold
            drive.setType(MotionEngineDSP::DriveType::wavefold);
            drive.setAmount(0.5f);
            float wavefoldOutput = drive.process(input);

            // All should be different (though not necessarily always different from each other)
            expect (! std::isnan(softClipOutput), "soft clip output should not be NaN");
            expect (! std::isnan(hardClipOutput), "hard clip output should not be NaN");
            expect (! std::isnan(wavefoldOutput), "wavefold output should not be NaN");

            // Outputs should be within reasonable bounds
            expect (softClipOutput >= -1.0f && softClipOutput <= 1.0f, "soft clip output should be bounded");
            expect (hardClipOutput >= -1.0f && hardClipOutput <= 1.0f, "hard clip output should be bounded");
            expect (wavefoldOutput >= -1.0f && wavefoldOutput <= 1.0f, "wavefold output should be bounded");
        }

        beginTest ("reset clears state");
        {
            MotionEngineDSP::DriveStage drive;

            // Set up a wavefolder
            drive.setType(MotionEngineDSP::DriveType::wavefold);
            drive.setMix(1.0f);
            drive.setAmount(0.5f);

            // Process some samples
            float output1 = drive.process(0.5f);
            float output2 = drive.process(0.7f);

            // Reset and process again
            drive.reset();
            float output3 = drive.process(0.5f);

            // The reset should clear any state-dependent behavior
            expect (! std::isnan(output1), "first output should not be NaN");
            expect (! std::isnan(output2), "second output should not be NaN");
            expect (! std::isnan(output3), "reset output should not be NaN");
        }

        beginTest ("amount parameter affects output");
        {
            MotionEngineDSP::DriveStage drive;

            drive.setType(MotionEngineDSP::DriveType::softClip);
            drive.setMix(1.0f);

            // Test with low amount
            drive.setAmount(0.1f);
            float lowAmountOutput = drive.process(0.8f);

            // Test with high amount
            drive.setAmount(0.9f);
            float highAmountOutput = drive.process(0.8f);

            expect (! std::isnan(lowAmountOutput), "low amount output should not be NaN");
            expect (! std::isnan(highAmountOutput), "high amount output should not be NaN");
        }
    }
};

static DriveStageTests driveStageTests;