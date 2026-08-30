#pragma once

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

        void setRate(float rateHz);
        void setShape(LFOShape shape);
        void setPhaseOffset(float phaseOffset);
        void setDepth(float depth);
        void reset();
        float process();

    private:
        float rate = 0.0f;
        LFOShape shape = LFOShape::sine;
        float phaseOffset = 0.0f;
        float depth = 0.0f;
        float phase = 0.0f;
        float lastOutput = 0.0f;
    };
}