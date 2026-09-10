#pragma once

#include <cmath>

namespace MotionEngineDSP
{

class TransientDetector
{
public:
    TransientDetector() = default;
    ~TransientDetector() = default;

    void reset()
    {
        refractoryCounter = 0;
        fastEnvelope = 0.0f;
        slowEnvelope = 0.0f;
    }

    bool process(float input)
    {
        // Update the refractory counter
        if (refractoryCounter > 0)
        {
            --refractoryCounter;
        }

        // Rectify the input
        const float rectified = std::abs(input);

        // Calculate coefficients for fast and slow envelope following
        // Fast envelope: attack = 1ms, release = 10ms (typical for transient detection)
        const float alphaFastAttack = 1.0f - std::exp(-1.0f / (1.0f * 0.001f * 44100.0f));
        const float alphaFastRelease = 1.0f - std::exp(-1.0f / (10.0f * 0.001f * 44100.0f));

        // Slow envelope: attack = 50ms, release = 200ms (typical for sustain detection)
        const float alphaSlowAttack = 1.0f - std::exp(-1.0f / (50.0f * 0.001f * 44100.0f));
        const float alphaSlowRelease = 1.0f - std::exp(-1.0f / (200.0f * 0.001f * 44100.0f));

        // Update fast envelope
        if (rectified > fastEnvelope)
        {
            fastEnvelope = fastEnvelope + alphaFastAttack * (rectified - fastEnvelope);
        }
        else
        {
            fastEnvelope = fastEnvelope + alphaFastRelease * (rectified - fastEnvelope);
        }

        // Update slow envelope
        if (rectified > slowEnvelope)
        {
            slowEnvelope = slowEnvelope + alphaSlowAttack * (rectified - slowEnvelope);
        }
        else
        {
            slowEnvelope = slowEnvelope + alphaSlowRelease * (rectified - slowEnvelope);
        }

        // Calculate the difference between fast and slow envelopes
        const float envelopeDifference = fastEnvelope - slowEnvelope;

        // If we're in the refractory period, don't trigger
        if (refractoryCounter > 0)
        {
            return false;
        }

        // Check if the difference exceeds threshold
        if (envelopeDifference > threshold)
        {
            // Trigger on onset and enter refractory period
            refractoryCounter = refractoryPeriod;
            return true;
        }

        return false;
    }

    void setThreshold(float newThreshold)
    {
        threshold = newThreshold;
    }

    void setRefractoryPeriod(int samples)
    {
        refractoryPeriod = samples;
    }

private:
    float threshold = 0.1f;
    int refractoryPeriod = 4410; // 100ms at 44.1kHz
    int refractoryCounter = 0;
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
};

} // namespace MotionEngineDSP