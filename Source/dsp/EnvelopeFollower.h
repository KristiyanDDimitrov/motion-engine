#pragma once

namespace MotionEngineDSP
{
    class EnvelopeFollower
    {
    public:
        EnvelopeFollower() = default;
        ~EnvelopeFollower() = default;

        void setAttack(float attackMs);
        void setRelease(float releaseMs);
        void setSensitivity(float sensitivity);
        float process(float input);

    private:
        float attackTime = 0.0f;
        float releaseTime = 0.0f;
        float sensitivity = 0.0f;
        float envelope = 0.0f;
    };
}