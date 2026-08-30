#pragma once

namespace MotionEngineDSP
{
    enum class FilterMode
    {
        lowpass,
        bandpass,
        highpass,
        notch
    };

    class FilterStage
    {
    public:
        FilterStage() = default;
        ~FilterStage() = default;

        void setMode(FilterMode mode);
        void setCutoff(float cutoffHz);
        void setResonance(float resonance);
        float process(float input);

    private:
        FilterMode mode = FilterMode::lowpass;
        float cutoff = 20.0f;
        float resonance = 1.0f;
        float state[4] = {0.0f}; // For state-variable filter
    };
}