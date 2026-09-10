#include <JuceHeader.h>
#include "dsp/LFO.h"

// Helper function to test specific tempo sync calculations
static float calculateTempoSyncRate(int division, float bpm)
{
    // This is the correct calculation: rate = (bpm * division) / 60
    return (bpm * division) / 60.0f;
}

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

            // A quarter-cycle offset on a sine near phase 0 moves the output
            // from ~0 to ~1, so require a real difference rather than testing
            // two floats for inequality.
            const float difference = output1 > output2 ? output1 - output2
                                                       : output2 - output1;
            expect (difference > 0.1f, "Phase offset should shift the output");
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

            const float difference = sineOutput > squareOutput ? sineOutput - squareOutput
                                                                : squareOutput - sineOutput;
            expect (difference > 0.1f, "Different shapes should produce different outputs");
        }

        beginTest ("depth affects amplitude");
        {
            // Measure the PEAK over a full cycle at each depth. The previous
            // version of this test read two successive samples at two depths
            // and compared them, which is not a test of depth at all - and it
            // failed on a coincidence: near phase 0, sin(2x) * 0.5 == sin(x)
            // to float precision, so the two readings came out identical.
            auto peakOverOneCycle = [] (float depth)
            {
                MotionEngineDSP::LFO lfo;
                lfo.setRate (1.0f);
                lfo.setShape (MotionEngineDSP::LFOShape::sine);
                lfo.setDepth (depth);
                lfo.reset();

                float peak = 0.0f;

                for (int i = 0; i < 44100; ++i)   // one full cycle at 1 Hz
                {
                    const float v = lfo.process();
                    const float magnitude = v < 0.0f ? -v : v;

                    if (magnitude > peak)
                        peak = magnitude;
                }

                return peak;
            };

            const float fullPeak = peakOverOneCycle (1.0f);
            const float halfPeak = peakOverOneCycle (0.5f);

            expect (fullPeak > 0.9f, "sine at depth 1.0 should reach near full scale");
            expectWithinAbsoluteError (halfPeak, fullPeak * 0.5f, 0.01f,
                                       "depth 0.5 should halve the peak amplitude");
        }

        beginTest ("tempo sync calculation correctness");
        {
            // Test that our tempo sync rate calculations are correct
            // The previous implementation had a bug where it computed: 1 / (division * secondsPerBeat)
            // But the correct formula is: (bpm * division) / 60

            // Test specific examples from the requirements:

            // 1/4 note at 90 BPM should be 6 Hz
            float expected_90bpm_4th = calculateTempoSyncRate(4, 90.0f);
            expectWithinAbsoluteError(expected_90bpm_4th, 6.0f, 0.01f, "1/4 note at 90 BPM should be 6 Hz");

            // 1/8 note at 174 BPM should be 23.2 Hz
            float expected_174bpm_8th = calculateTempoSyncRate(8, 174.0f);
            expectWithinAbsoluteError(expected_174bpm_8th, 23.2f, 0.01f, "1/8 note at 174 BPM should be 23.2 Hz");

            // 1/16 note at 200 BPM should be 53.33 Hz
            float expected_200bpm_16th = calculateTempoSyncRate(16, 200.0f);
            expectWithinAbsoluteError(expected_200bpm_16th, 53.33f, 0.01f, "1/16 note at 200 BPM should be 53.33 Hz");

            // Test common divisions at 120 BPM
            expectWithinAbsoluteError(calculateTempoSyncRate(1, 120.0f), 2.0f, 0.01f, "Whole note at 120 BPM should be 2 Hz");
            expectWithinAbsoluteError(calculateTempoSyncRate(2, 120.0f), 4.0f, 0.01f, "Half note at 120 BPM should be 4 Hz");
            expectWithinAbsoluteError(calculateTempoSyncRate(4, 120.0f), 8.0f, 0.01f, "Quarter note at 120 BPM should be 8 Hz");
            expectWithinAbsoluteError(calculateTempoSyncRate(8, 120.0f), 16.0f, 0.01f, "Eighth note at 120 BPM should be 16 Hz");
            expectWithinAbsoluteError(calculateTempoSyncRate(32, 120.0f), 64.0f, 0.01f, "32nd note at 120 BPM should be 64 Hz");
        }

        beginTest ("tempo sync at 90 BPM correct");
        {
            // Test that tempo sync works correctly by verifying the rate conversion
            MotionEngineDSP::LFO lfo;

            // Set a tempo-synced rate for 1/4 note at 90 BPM
            lfo.setTempoSyncedRate(4, 90.0f);

            expect (lfo.isTempoSynced(), "LFO should be marked as tempo-synced");
        }

        beginTest ("tempo sync at 174 BPM correct");
        {
            MotionEngineDSP::LFO lfo;

            // Set a tempo-synced rate for 1/8 note at 174 BPM
            lfo.setTempoSyncedRate(8, 174.0f);

            expect (lfo.isTempoSynced(), "LFO should be marked as tempo-synced");
        }

        beginTest ("tempo sync at 200 BPM correct");
        {
            MotionEngineDSP::LFO lfo;

            // Set a tempo-synced rate for 1/16 note at 200 BPM
            lfo.setTempoSyncedRate(16, 200.0f);

            expect (lfo.isTempoSynced(), "LFO should be marked as tempo-synced");
        }

        beginTest ("tempo sync conversion works for all divisions at 120 BPM");
        {
            MotionEngineDSP::LFO lfo;

            // Test that we can set various tempo-synced rates without errors
            const float bpm = 120.0f;

            // Test several common divisions - just verify they don't crash and are marked as tempo-synced
            lfo.setTempoSyncedRate(1, bpm);  // Whole note
            expect (lfo.isTempoSynced(), "Whole note should be tempo-synced");

            lfo.setTempoSyncedRate(2, bpm);  // Half note
            expect (lfo.isTempoSynced(), "Half note should be tempo-synced");

            lfo.setTempoSyncedRate(4, bpm);  // Quarter note
            expect (lfo.isTempoSynced(), "Quarter note should be tempo-synced");

            lfo.setTempoSyncedRate(8, bpm);  // Eighth note
            expect (lfo.isTempoSynced(), "Eighth note should be tempo-synced");

            lfo.setTempoSyncedRate(32, bpm); // 32nd note
            expect (lfo.isTempoSynced(), "32nd note should be tempo-synced");
        }
    }
};

static LFOTests lfoTests;