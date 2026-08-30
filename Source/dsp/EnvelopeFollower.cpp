#include "EnvelopeFollower.h"

namespace MotionEngineDSP
{
    void EnvelopeFollower::setAttack(float attackMs)
    {
        attackTime = attackMs;
    }

    void EnvelopeFollower::setRelease(float releaseMs)
    {
        releaseTime = releaseMs;
    }

    void EnvelopeFollower::setSensitivity(float sensitivity)
    {
        this->sensitivity = sensitivity;
    }

    float EnvelopeFollower::process(float input)
    {
        // Simple envelope follower implementation
        float absInput = std::abs(input);

        if (absInput > envelope)
        {
            // Attack phase
            envelope += (absInput - envelope) * (1.0f - std::exp(-1000.0f / (attackTime * 44100.0f)));
        }
        else
        {
            // Release phase
            envelope += (absInput - envelope) * (1.0f - std::exp(-1000.0f / (releaseTime * 44100.0f)));
        }

        return envelope;
    }
}