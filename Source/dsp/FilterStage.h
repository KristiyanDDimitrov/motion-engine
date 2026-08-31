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

    void reset();
    float process(float input);
    void setMode(FilterMode mode);
    void setCutoff(float cutoffHz);
    void setResonance(float resonance);

private:
    FilterMode mode = FilterMode::lowpass;
    float cutoffHz = 1000.0f;
    float resonance = 1.0f;

    // State variables for state-variable filter
    float lowpass = 0.0f;
    float bandpass = 0.0f;
    float highpass = 0.0f;
    float notch = 0.0f;
};

} // namespace MotionEngineDSP