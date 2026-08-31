#pragma once

namespace MotionEngineDSP
{

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;
    ~EnvelopeFollower() = default;

    void reset();
    float process(float input);
    void setAttack(float attackMs);
    void setRelease(float releaseMs);
    void setSensitivity(float sensitivity);

private:
    float attackMs = 10.0f;
    float releaseMs = 50.0f;
    float sensitivity = 0.5f;
    float envelope = 0.0f;
};

} // namespace MotionEngineDSP