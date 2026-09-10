#pragma once

#include <array>
#include <algorithm>

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

    void reset()
    {
        // Reset all slots to defaults (constant source, 0 depth)
        for (auto& slot : slots)
        {
            slot.source = ModMatrixSource::constant;
            slot.destination = ModMatrixDestination::filterCutoff;
            slot.depth = 0.0f;
        }

        // Reset constant values
        for (auto& value : constantValues)
        {
            value = 0.0f;
        }
    }

    // Process all modulation slots and return the total modulation for each destination
    struct ModulationResult
    {
        float filterCutoff = 0.0f;
        float filterResonance = 0.0f;
        float driveAmount = 0.0f;
        float outputGain = 0.0f;
        float lfo2Rate = 0.0f;
    };

    ModulationResult process(float envelopeValue, bool transientTrigger, float lfo1Output, float lfo2Output)
    {
        ModulationResult result{};

        // Process each slot
        for (const auto& slot : slots)
        {
            // Skip empty slots (depth = 0) - this is a no-op
            if (slot.depth == 0.0f)
                continue;

            float inputValue = 0.0f;

            // Get input value based on source type
            switch (slot.source)
            {
                case ModMatrixSource::envelope:
                    inputValue = envelopeValue;
                    break;

                case ModMatrixSource::transient:
                    inputValue = transientTrigger ? 1.0f : 0.0f;
                    break;

                case ModMatrixSource::lfo1:
                    inputValue = lfo1Output;
                    break;

                case ModMatrixSource::lfo2:
                    inputValue = lfo2Output;
                    break;

                case ModMatrixSource::macro:
                    // Use the constant value for macro sources
                    inputValue = constantValues[static_cast<int>(ModMatrixSource::macro)];
                    break;

                case ModMatrixSource::constant:
                    // Use the constant value for constant source
                    inputValue = constantValues[static_cast<int>(ModMatrixSource::constant)];
                    break;
            }

            // Apply modulation depth and accumulate to destination
            float contribution = inputValue * slot.depth;

            switch (slot.destination)
            {
                case ModMatrixDestination::filterCutoff:
                    result.filterCutoff += contribution;
                    break;

                case ModMatrixDestination::filterResonance:
                    result.filterResonance += contribution;
                    break;

                case ModMatrixDestination::driveAmount:
                    result.driveAmount += contribution;
                    break;

                case ModMatrixDestination::outputGain:
                    result.outputGain += contribution;
                    break;

                case ModMatrixDestination::lfo2Rate:
                    result.lfo2Rate += contribution;
                    break;
            }
        }

        return result;
    }

    // Set modulation slot parameters
    void setSlot(int slotIndex, ModMatrixSource source, ModMatrixDestination destination, float depth)
    {
        if (slotIndex >= 0 && slotIndex < static_cast<int>(slots.size()))
        {
            slots[slotIndex].source = source;
            slots[slotIndex].destination = destination;
            slots[slotIndex].depth = depth;
        }
    }

    // Set constant values for macro sources
    void setConstantValue(ModMatrixSource source, float value)
    {
        int index = static_cast<int>(source);
        if (index >= 0 && index < static_cast<int>(constantValues.size()))
        {
            constantValues[index] = value;
        }
    }

private:
    struct Slot
    {
        ModMatrixSource source = ModMatrixSource::constant;
        ModMatrixDestination destination = ModMatrixDestination::filterCutoff;
        float depth = 0.0f;
    };

    std::array<Slot, 4> slots;
    std::array<float, 6> constantValues; // For macro sources (and constant)
};

} // namespace MotionEngineDSP