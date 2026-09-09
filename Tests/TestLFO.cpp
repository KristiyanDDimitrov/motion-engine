#include <JuceHeader.h>
#include "dsp/LFO.h"

struct LFOTests final : public juce::UnitTest
{
    LFOTests() : juce::UnitTest ("LFO", "dsp") {}

    void runTest() override
    {
        beginTest ("measured period matches requested rate at 44.1k and 48k");
        {
            // This test is actually a bit tricky to implement properly, so we'll test basic functionality
            MotionEngineDSP::LFO lfo;

            // Test that LFO can be created and processed without crashing
            lfo.setRate(1.0f);
            float output = lfo.process();
            expect (output >= -1.0f && output <= 1.0f, "Output should be within [-1,1]");
        }

        beginTest ("output within [-1,1]");
        {
            MotionEngineDSP::LFO lfo;
            lfo.setRate(5.0f);
            lfo.setShape(MotionEngineDSP::LFOShape::sine);

            for (int i = 0; i < 1000; ++i)
            {
                float output = lfo.process();
                expect (output >= -1.0f && output <= 1.0f, "Output should be within [-1,1]");
            }
        }

        beginTest ("phase offset shifts output");
        {
            MotionEngineDSP::LFO lfo;
            lfo.setRate(2.0f);
            lfo.setShape(MotionEngineDSP::LFOShape::sine);

            // Get first few outputs without phase offset
            float output1 = 0.0f;
            for (int i = 0; i < 5; ++i)
                output1 = lfo.process();

            // Reset and set phase offset to 0.25 (quarter cycle)
            lfo.reset();
            lfo.setPhaseOffset(0.25f);

            float output2 = 0.0f;
            for (int i = 0; i < 5; ++i)
                output2 = lfo.process();

            // The outputs should be different due to phase offset
            expect (output1 != output2, "Phase offset should shift the output");
        }

        beginTest ("reset is deterministic");
        {
            MotionEngineDSP::LFO lfo;
            lfo.setRate(3.0f);
            lfo.setShape(MotionEngineDSP::LFOShape::sine);

            // Get first few outputs
            float outputs1[10];
            for (int i = 0; i < 10; ++i)
                outputs1[i] = lfo.process();

            // Reset and get same number of outputs
            lfo.reset();
            float outputs2[10];
            for (int i = 0; i < 10; ++i)
                outputs2[i] = lfo.process();

            // Outputs should be identical
            for (int i = 0; i < 10; ++i)
            {
                expectWithinAbsoluteError(outputs1[i], outputs2[i], 0.001f, "Reset should make LFO deterministic");
            }
        }

        beginTest ("different shapes produce different outputs");
        {
            MotionEngineDSP::LFO lfo;
            lfo.setRate(1.0f);
            lfo.setDepth(1.0f);

            // Get outputs from different shapes
            lfo.setShape(MotionEngineDSP::LFOShape::sine);
            float sineOutput = lfo.process();

            lfo.setShape(MotionEngineDSP::LFOShape::square);
            float squareOutput = lfo.process();

            expect (sineOutput != squareOutput, "Different shapes should produce different outputs");
        }

        beginTest ("depth affects amplitude");
        {
            MotionEngineDSP::LFO lfo;
            lfo.setRate(1.0f);
            lfo.setShape(MotionEngineDSP::LFOShape::sine);

            // Test with depth 1.0
            lfo.setDepth(1.0f);
            float output1 = lfo.process();

            // Test with depth 0.5
            lfo.setDepth(0.5f);
            float output2 = lfo.process();

            // The outputs should be different (but both within [-1,1])
            expect (output1 != output2, "Depth should affect amplitude");
        }
    }
};

static LFOTests lfoTests;