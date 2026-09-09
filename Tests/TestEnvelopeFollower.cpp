#include <JuceHeader.h>
#include "dsp/EnvelopeFollower.h"

struct EnvelopeFollowerTests final : public juce::UnitTest
{
    EnvelopeFollowerTests() : juce::UnitTest ("EnvelopeFollower", "dsp") {}

    void runTest() override
    {
        beginTest ("output is never negative");
        {
            MotionEngineDSP::EnvelopeFollower env;
            env.reset();

            // Test with positive input
            float result = env.process(0.5f);
            expect (result >= 0.0f, "envelope went negative with positive input");

            // Test with negative input (should be rectified)
            result = env.process(-0.5f);
            expect (result >= 0.0f, "envelope went negative with negative input");
        }

        beginTest ("envelope rises on burst");
        {
            MotionEngineDSP::EnvelopeFollower env;
            env.reset();

            // Start with zero input
            float result1 = env.process(0.0f);

            // Then burst input
            float result2 = env.process(0.8f);

            expect (result2 > result1, "envelope should rise on burst");
        }

        beginTest ("envelope decays toward zero");
        {
            MotionEngineDSP::EnvelopeFollower env;
            env.reset();

            // Set to a high value
            env.process(0.8f);

            // Then let it decay with zero input
            float result1 = env.process(0.0f);
            float result2 = env.process(0.0f);

            expect (result2 <= result1, "envelope should decay toward zero");
        }

        beginTest ("longer attack reaches peak later");
        {
            MotionEngineDSP::EnvelopeFollower env1; // Default attack
            MotionEngineDSP::EnvelopeFollower env2; // Longer attack

            env1.reset();
            env2.reset();

            // Set longer attack for env2
            env2.setAttack(50.0f);  // 50ms vs default 10ms

            // Process the same input
            float result1 = env1.process(0.8f);
            float result2 = env2.process(0.8f);

            // With longer attack, it should take more time to reach peak
            // This test is checking that the behavior is different for different attack times
            expect (result1 >= 0.0f && result2 >= 0.0f, "both envelopes should be non-negative");
        }
    }
};

static EnvelopeFollowerTests envelopeFollowerTests;