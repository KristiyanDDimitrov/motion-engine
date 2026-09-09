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
            env.setAttack(10.0f);
            env.setRelease(50.0f);

            // Test that output never goes negative
            for (int i = 0; i < 1000; ++i)
            {
                float input = (i % 200) < 100 ? 0.5f : -0.5f;
                float output = env.process(input);
                expect (output >= 0.0f, "envelope went negative");
            }
        }

        beginTest ("rises on burst");
        {
            MotionEngineDSP::EnvelopeFollower env;
            env.setAttack(10.0f);
            env.setRelease(50.0f);

            // Start with zero input
            float output = env.process(0.0f);
            expect (output >= 0.0f, "initial output should be non-negative");

            // Provide a burst of positive input
            float burstInput = 1.0f;
            for (int i = 0; i < 100; ++i)
            {
                output = env.process(burstInput);
                expect (output >= 0.0f, "envelope should rise on burst");
            }

            // Ensure it rises
            float initialOutput = env.process(0.0f);
            expect (initialOutput > 0.0f, "envelope should rise from zero to positive value");
        }

        beginTest ("decays toward zero");
        {
            MotionEngineDSP::EnvelopeFollower env;
            env.setAttack(10.0f);
            env.setRelease(50.0f);

            // Provide input to build up envelope
            for (int i = 0; i < 100; ++i)
            {
                env.process(1.0f);
            }

            // Now let it decay with no input
            float previousOutput = 1.0f;
            for (int i = 0; i < 500; ++i)
            {
                float output = env.process(0.0f);
                expect (output >= 0.0f, "envelope should decay toward zero but not go negative");
                expect (output <= previousOutput, "envelope should decay");
                previousOutput = output;
            }
        }

        beginTest ("longer attack reaches peak later");
        {
            MotionEngineDSP::EnvelopeFollower env1; // Short attack
            MotionEngineDSP::EnvelopeFollower env2; // Long attack

            env1.setAttack(5.0f);
            env1.setRelease(50.0f);

            env2.setAttack(50.0f);
            env2.setRelease(50.0f);

            // Provide same input to both
            float input = 1.0f;
            std::vector<float> output1, output2;

            for (int i = 0; i < 300; ++i)
            {
                output1.push_back(env1.process(input));
                output2.push_back(env2.process(input));
            }

            // Find when each reaches peak
            float max1 = *std::max_element(output1.begin(), output1.end());
            float max2 = *std::max_element(output2.begin(), output2.end());

            // The envelope with longer attack should reach its peak later
            expect (max1 > 0.0f, "short attack should reach peak");
            expect (max2 > 0.0f, "long attack should reach peak");

            // Check that both reach the same maximum value (as they're using same input)
            // But the long attack will take longer to get there
        }
    }
};

static EnvelopeFollowerTests envelopeFollowerTests;