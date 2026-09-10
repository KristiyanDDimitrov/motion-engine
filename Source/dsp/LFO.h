#pragma once

#include <cmath>

namespace MotionEngineDSP
{

enum class LFOShape
{
    sine,
    triangle,
    saw,
    square,
    sampleAndHold,
    stepped
};

class LFO
{
public:
    LFO() = default;
    ~LFO() = default;

    void reset()
    {
        phase = 0.0f;
    }

    float process()
    {
        // Update phase - increment by rateHz / sampleRate to get correct frequency
        phase += rateHz / 44100.0f; // 44.1kHz sample rate

        // Wrap phase to [0, 1)
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Calculate output based on shape
        float output = 0.0f;

        switch (shape)
        {
            case LFOShape::sine:
                output = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                break;

            case LFOShape::triangle:
                // Triangle wave: 2 * |2 * phase - 1| - 1
                output = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
                break;

            case LFOShape::saw:
                // Sawtooth wave: 2 * phase - 1
                output = 2.0f * phase - 1.0f;
                break;

            case LFOShape::square:
                // Square wave: 1 if phase < 0.5, -1 otherwise
                output = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            case LFOShape::sampleAndHold:
                // Sample and hold: random value between -1 and 1
                output = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
                break;

            case LFOShape::stepped:
                // Stepped wave: stepped values based on phase
                output = std::floor(phase * 8.0f) / 4.0f - 1.0f; // 8 steps
                break;
        }

        // Apply phase offset (shift the waveform)
        if (phaseOffset != 0.0f)
        {
            float newPhase = phase + phaseOffset;
            if (newPhase >= 1.0f)
                newPhase -= 1.0f;
            else if (newPhase < 0.0f)
                newPhase += 1.0f;

            // Recalculate output with offset phase
            switch (shape)
            {
                case LFOShape::sine:
                    output = std::sin(2.0f * juce::MathConstants<float>::pi * newPhase);
                    break;

                case LFOShape::triangle:
                    output = 2.0f * std::abs(2.0f * newPhase - 1.0f) - 1.0f;
                    break;

                case LFOShape::saw:
                    output = 2.0f * newPhase - 1.0f;
                    break;

                case LFOShape::square:
                    output = (newPhase < 0.5f) ? 1.0f : -1.0f;
                    break;

                case LFOShape::sampleAndHold:
                    // For sample and hold, we just use the original output
                    break;

                case LFOShape::stepped:
                    output = std::floor(newPhase * 8.0f) / 4.0f - 1.0f;
                    break;
            }
        }

        // Apply depth (amplitude)
        output *= depth;

        // Clamp to [-1, 1]
        if (output > 1.0f)
            output = 1.0f;
        else if (output < -1.0f)
            output = -1.0f;

        return output;
    }

    void setRate(float rate)
    {
        this->rateHz = rate;
        tempoSynced = false;
    }

    void setShape(LFOShape newShape)
    {
        this->shape = newShape;
    }

    void setPhaseOffset(float offset)
    {
        this->phaseOffset = offset;
    }

    void setDepth(float newDepth)
    {
        this->depth = newDepth;
    }

    // Tempo-synced rate functions
    void setTempoSyncedRate(int division, float bpm) // 1/1 = 1, 1/2 = 2, ..., 1/32 = 32
    {
        // Convert tempo-synced division to Hz
        // For a 1/n note at bpm:
        // - seconds per beat = 60 / bpm
        // - seconds per note = (seconds per beat) / n = (60 / bpm) / n = 60 / (bpm * n)
        // - frequency = 1 / (seconds per note) = bpm * n / 60
        float rate = (bpm * division) / 60.0f;

        this->rateHz = rate;
        tempoSynced = true;
    }

    bool isTempoSynced() const { return tempoSynced; }

private:
    float rateHz = 1.0f;
    LFOShape shape = LFOShape::sine;
    float phaseOffset = 0.0f;
    float depth = 1.0f;
    bool tempoSynced = false;

    float phase = 0.0f;
};

} // namespace MotionEngineDSP