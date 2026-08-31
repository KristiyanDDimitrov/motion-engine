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

    void reset();
    float process();
    void setRate(float rateHz);
    void setShape(LFOShape shape);
    void setPhaseOffset(float phaseOffset);
    void setDepth(float depth);

    // Tempo-synced rate functions
    void setTempoSyncedRate(int division); // 1/1 = 1, 1/2 = 2, ..., 1/32 = 32
    bool isTempoSynced() const { return tempoSynced; }

private:
    float rateHz = 1.0f;
    LFOShape shape = LFOShape::sine;
    float phaseOffset = 0.0f;
    float depth = 1.0f;
    bool tempoSynced = false;
    int tempoDivision = 4; // Default to 1/4 note

    float phase = 0.0f;
    float lastOutput = 0.0f;
};

} // namespace MotionEngineDSP