#pragma once

namespace MotionEngineDSP
{
    class TransientDetector
    {
    public:
        TransientDetector() = default;
        ~TransientDetector() = default;

        void setThreshold(float threshold);
        void setRefractoryPeriod(int samples);
        bool process(float input);

    private:
        float threshold = 0.0f;
        int refractoryPeriod = 0;
        int refractoryCounter = 0;
        float fastEnvelope = 0.0f;
        float slowEnvelope = 0.0f;
    };
}