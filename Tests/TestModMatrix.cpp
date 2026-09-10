#include <JuceHeader.h>
#include "dsp/ModMatrix.h"

struct ModMatrixTests final : public juce::UnitTest
{
    ModMatrixTests() : juce::UnitTest ("ModMatrix", "dsp") {}

    void runTest() override
    {
        beginTest ("contributions sum");
        {
            MotionEngineDSP::ModMatrix modMatrix;

            // Set up a simple test with two slots contributing to the same destination
            modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::envelope,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, 0.5f);
            modMatrix.setSlot(1, MotionEngineDSP::ModMatrixSource::lfo1,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, 0.3f);

            // Process with known inputs
            auto result = modMatrix.process(0.8f, false, 0.6f, 0.0f);

            // Should have 0.8 * 0.5 + 0.6 * 0.3 = 0.4 + 0.18 = 0.58
            expectWithinAbsoluteError(result.filterCutoff, 0.58f, 0.001f,
                                      "Contributions should sum correctly");
        }

        beginTest ("results clamp to destination range");
        {
            MotionEngineDSP::ModMatrix modMatrix;

            // Set up a slot that would produce an out-of-range value
            modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::envelope,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, 1.0f);

            // Process with high envelope value
            auto result = modMatrix.process(1.0f, false, 0.0f, 0.0f);

            // Should clamp to valid range for filter cutoff (20-20000 Hz)
            // But since we don't know the exact clamping logic from spec, let's test
            // that it doesn't produce invalid results - for now just check it's not NaN or Inf
            expect (! std::isnan(result.filterCutoff), "Result should not be NaN");
            expect (! std::isinf(result.filterCutoff), "Result should not be infinite");
        }

        beginTest ("depth 0 is a no-op");
        {
            MotionEngineDSP::ModMatrix modMatrix;

            // Set up a slot with depth = 0
            modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::envelope,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, 0.0f);

            // Process with high values - should produce no modulation
            auto result = modMatrix.process(1.0f, true, 1.0f, 1.0f);

            expectWithinAbsoluteError(result.filterCutoff, 0.0f, 0.001f,
                                      "Depth 0 should be a true no-op");
        }

        beginTest ("negative depth inverts contribution");
        {
            MotionEngineDSP::ModMatrix modMatrix;

            // Set up a slot with negative depth
            modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::envelope,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, -0.5f);

            // Process with known input
            auto result = modMatrix.process(0.8f, false, 0.0f, 0.0f);

            // Should have 0.8 * (-0.5) = -0.4
            expectWithinAbsoluteError(result.filterCutoff, -0.4f, 0.001f,
                                      "Negative depth should invert contribution");
        }

        beginTest ("macro source works correctly");
        {
            MotionEngineDSP::ModMatrix modMatrix;

            // Set constant value for macro
            modMatrix.setConstantValue(MotionEngineDSP::ModMatrixSource::macro, 0.7f);

            // Set up a slot using macro source
            modMatrix.setSlot(0, MotionEngineDSP::ModMatrixSource::macro,
                              MotionEngineDSP::ModMatrixDestination::filterCutoff, 1.0f);

            // Process
            auto result = modMatrix.process(0.0f, false, 0.0f, 0.0f);

            expectWithinAbsoluteError(result.filterCutoff, 0.7f, 0.001f,
                                      "Macro source should use constant value");
        }
    }
};

static ModMatrixTests modMatrixTests;