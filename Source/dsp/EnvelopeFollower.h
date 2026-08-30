#pragma once

namespace MotionEngineDSP
{
    class EnvelopeFollower
    {
    public:
        EnvelopeFollower();

        void setSampleRate(double sampleRate);
        void setAttack(float attackMs);
        void setRelease(float releaseMs);
        void setSensitivity(float sensitivity);

        float process(float input);
        void reset();

    private:
        double sampleRate = 44100.0;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        float sensitivity = 0.5f;
        float envelope = 0.0f;

        void updateCoefficients();
    };
}