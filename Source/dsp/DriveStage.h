#pragma once

namespace MotionEngineDSP
{

enum class DriveType
{
    softClip,
    hardClip,
    wavefold
};

class DriveStage
{
public:
    DriveStage() = default;
    ~DriveStage() = default;

    void reset()
    {
        lastInput = 0.0f;
    }

    float process(float input)
    {
        if (mix == 0.0f)
        {
            // Bypass - return clean input
            return input;
        }

        float output = input;

        switch (type)
        {
            case DriveType::softClip:
            {
                // Soft clip: smooth clipping with a gradual transition
                const float k = amount * 2.0f; // Scale the amount
                if (k > 0.0f)
                {
                    output = input / (1.0f + k * std::abs(input));
                }
                break;
            }

            case DriveType::hardClip:
            {
                // Hard clip: abrupt clipping at ±1.0
                const float k = amount; // Scale the amount
                if (k > 0.0f)
                {
                    output = std::max(-1.0f, std::min(1.0f, input));
                }
                break;
            }

            case DriveType::wavefold:
            {
                // Wavefolder: creates harmonic content by folding the waveform
                const float k = amount * 0.5f; // Scale the amount
                if (k > 0.0f)
                {
                    // Fold the waveform based on last input
                    output = std::sin(input + k * lastInput);
                    lastInput = input;
                }
                break;
            }
        }

        // Apply mix: blend between clean and driven signal
        return (1.0f - mix) * input + mix * output;
    }

    void setAmount(float newAmount)
    {
        amount = newAmount;
    }

    void setType(DriveType newType)
    {
        type = newType;
    }

    void setMix(float newMix)
    {
        mix = newMix;
    }

private:
    float amount = 0.5f;
    DriveType type = DriveType::softClip;
    float mix = 0.0f; // Default to bypass

    // State for wavefolder
    float lastInput = 0.0f;
};

} // namespace MotionEngineDSP