#pragma once

#include <cmath>

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

    void reset()
    {
        // Reset all state variables to zero
        lowpass = 0.0f;
        bandpass = 0.0f;
        highpass = 0.0f;
        notch = 0.0f;
    }

    float process(float input)
    {
        // Calculate filter coefficients
        const float g = std::tan(M_PI * cutoffHz / 44100.0f); // Using 44.1kHz as default sample rate
        const float k = 1.0f / resonance;

        // State-variable filter calculation
        float output = 0.0f;

        switch (mode)
        {
            case FilterMode::lowpass:
                lowpass += g * bandpass;
                highpass = input - lowpass - k * bandpass;
                bandpass += g * highpass;
                output = lowpass;
                break;

            case FilterMode::bandpass:
                lowpass += g * bandpass;
                highpass = input - lowpass - k * bandpass;
                bandpass += g * highpass;
                output = bandpass;
                break;

            case FilterMode::highpass:
                lowpass += g * bandpass;
                highpass = input - lowpass - k * bandpass;
                bandpass += g * highpass;
                output = highpass;
                break;

            case FilterMode::notch:
                lowpass += g * bandpass;
                highpass = input - lowpass - k * bandpass;
                bandpass += g * highpass;
                output = lowpass + highpass;
                break;
        }

        return output;
    }

    void setMode(FilterMode mode)
    {
        this->mode = mode;
    }

    void setCutoff(float cutoffHz)
    {
        // Clamp cutoff frequency to valid range
        this->cutoffHz = std::max(20.0f, std::min(20000.0f, cutoffHz));
    }

    void setResonance(float resonance)
    {
        // Clamp resonance to valid range
        this->resonance = std::max(0.1f, std::min(20.0f, resonance));
    }

    float getCutoff() const { return cutoffHz; }
    float getResonance() const { return resonance; }

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