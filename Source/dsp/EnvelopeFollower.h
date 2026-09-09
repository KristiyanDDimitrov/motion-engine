#pragma once

namespace MotionEngineDSP
{

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;
    ~EnvelopeFollower() = default;

    void reset()
    {
        envelope = 0.0f;
    }

    float process(float input)
    {
        // Rectify the input
        const float rectified = std::abs(input);

        // Calculate coefficients for attack and release
        // Using a simple exponential smoothing approach
        const float alphaAttack = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * 44100.0f)); // Assuming 44.1kHz sample rate
        const float alphaRelease = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * 44100.0f));

        // Apply attack or release based on whether we're rising or falling
        if (rectified > envelope)
        {
            envelope = envelope + alphaAttack * (rectified - envelope);
        }
        else
        {
            envelope = envelope + alphaRelease * (rectified - envelope);
        }

        // Apply sensitivity - this multiplies the output by sensitivity level
        return envelope * sensitivity;
    }

    void setAttack(float newAttackMs)
    {
        attackMs = juce::jlimit(0.1f, 200.0f, newAttackMs);
    }

    void setRelease(float newReleaseMs)
    {
        releaseMs = juce::jlimit(1.0f, 1000.0f, newReleaseMs);
    }

    void setSensitivity(float newSensitivity)
    {
        sensitivity = juce::jlimit(0.0f, 1.0f, newSensitivity);
    }

private:
    float attackMs = 10.0f;
    float releaseMs = 50.0f;
    float sensitivity = 1.0f;
    float envelope = 0.0f;
};

} // namespace MotionEngineDSP