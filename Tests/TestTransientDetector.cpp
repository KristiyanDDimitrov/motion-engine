#include <JuceHeader.h>
#include "dsp/TransientDetector.h"

struct TransientDetectorTests final : public juce::UnitTest
{
    TransientDetectorTests() : juce::UnitTest ("TransientDetector", "dsp") {}

    void runTest() override
    {
        beginTest ("exactly one trigger per burst onset");
        {
            MotionEngineDSP::TransientDetector detector;
            detector.setThreshold(0.1f);
            detector.setRefractoryPeriod(4410); // 100ms at 44.1kHz

            // Create a test signal with a single burst - using simple pattern
            std::vector<float> inputSignal = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Silence
                                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // Strong burst (should trigger)
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Silence

            bool triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    expect (!triggered, "Should only trigger once per burst");
                    triggered = true;
                }
            }

            expect (triggered, "Should have triggered on burst onset");
        }

        beginTest ("none during sustain");
        {
            MotionEngineDSP::TransientDetector detector;
            detector.setThreshold(0.1f);
            detector.setRefractoryPeriod(4410); // 100ms at 44.1kHz

            // Create a test signal with sustained input
            std::vector<float> inputSignal = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Silence
                                             0.5f, 0.5f, 0.5f, 0.5f, 0.5f,   // Sustained burst (should not trigger)
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Silence

            bool triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    expect (!triggered, "Should not trigger during sustained period");
                    triggered = true;
                }
            }

            expect (!triggered, "Should not have triggered during sustained period");
        }

        beginTest ("respects refractory period");
        {
            MotionEngineDSP::TransientDetector detector;
            detector.setThreshold(0.1f);
            detector.setRefractoryPeriod(4410); // 100ms at 44.1kHz

            // Create a test signal with two bursts
            std::vector<float> inputSignal = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Silence
                                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // First burst (should trigger)
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // Silence
                                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // Second burst (should NOT trigger due to refractory)
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Silence

            int triggerCount = 0;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    triggerCount++;
                }
            }

            expect (triggerCount == 1, "Should only trigger once during the first burst");
        }

        beginTest ("threshold affects triggering");
        {
            MotionEngineDSP::TransientDetector detector;
            detector.setRefractoryPeriod(4410); // 100ms at 44.1kHz

            // Test with a high threshold that should not trigger
            detector.setThreshold(2.0f); // Very high threshold
            std::vector<float> inputSignal = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Silence
                                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // Burst with amplitude 1.0
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Silence

            bool triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    triggered = true;
                }
            }

            expect (!triggered, "Should not trigger with high threshold");

            // Test with a low threshold that should trigger
            detector.setThreshold(0.1f); // Low threshold
            triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    triggered = true;
                }
            }

            expect (triggered, "Should trigger with low threshold");
        }

        beginTest ("reset clears state");
        {
            MotionEngineDSP::TransientDetector detector;
            detector.setThreshold(0.1f);
            detector.setRefractoryPeriod(4410); // 100ms at 44.1kHz

            // Create a burst and trigger
            std::vector<float> inputSignal = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Silence
                                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // Burst (should trigger)
                                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Silence

            bool triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    triggered = true;
                }
            }

            expect (triggered, "Should have triggered on burst");

            // Reset the detector
            detector.reset();

            // Create another burst after reset - should trigger again
            triggered = false;
            for (size_t i = 0; i < inputSignal.size(); ++i)
            {
                if (detector.process(inputSignal[i]))
                {
                    triggered = true;
                }
            }

            expect (triggered, "Should trigger after reset");
        }
    }
};

static TransientDetectorTests transientDetectorTests;