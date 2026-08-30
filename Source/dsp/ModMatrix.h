#pragma once

#include <array>

namespace MotionEngineDSP
{
    enum class ModSource
    {
        envelope,
        transient,
        lfo1,
        lfo2,
        macro,
        constant
    };

    enum class ModDestination
    {
        filterCutoff,
        filterResonance,
        driveAmount,
        outputGain,
        lfo2Rate
    };

    struct ModSlot
    {
        ModSource source = ModSource::constant;
        ModDestination destination = ModDestination::filterCutoff;
        float depth = 0.0f;
    };

    class ModMatrix
    {
    public:
        ModMatrix() = default;
        ~ModMatrix() = default;

        void setSlot(int slotIndex, const ModSlot& slot);
        float getModulation(ModDestination destination);

    private:
        std::array<ModSlot, 4> slots;
        float envelopeValue = 0.0f;
        bool transientTrigger = false;
        float lfo1Output = 0.0f;
        float lfo2Output = 0.0f;
        float macroValue = 0.0f;
        float constantValue = 0.0f;
    };
}