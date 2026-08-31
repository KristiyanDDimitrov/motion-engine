#pragma once

#include <array>

namespace MotionEngineDSP
{

enum class ModMatrixSource
{
    envelope,
    transient,
    lfo1,
    lfo2,
    macro,
    constant
};

enum class ModMatrixDestination
{
    filterCutoff,
    filterResonance,
    driveAmount,
    outputGain,
    lfo2Rate
};

class ModMatrix
{
public:
    ModMatrix() = default;
    ~ModMatrix() = default;

    void reset();

    // Process all modulation slots and return the total modulation for each destination
    struct ModulationResult
    {
        float filterCutoff = 0.0f;
        float filterResonance = 0.0f;
        float driveAmount = 0.0f;
        float outputGain = 0.0f;
        float lfo2Rate = 0.0f;
    };

    ModulationResult process(float envelopeValue, bool transientTrigger, float lfo1Output, float lfo2Output);

    // Set modulation slot parameters
    void setSlot(int slotIndex, ModMatrixSource source, ModMatrixDestination destination, float depth);

    // Set constant values for macro sources
    void setConstantValue(ModMatrixSource source, float value);

private:
    struct Slot
    {
        ModMatrixSource source = ModMatrixSource::constant;
        ModMatrixDestination destination = ModMatrixDestination::filterCutoff;
        float depth = 0.0f;
    };

    std::array<Slot, 4> slots;
    std::array<float, 6> constantValues; // For macro sources
};

} // namespace MotionEngineDSP