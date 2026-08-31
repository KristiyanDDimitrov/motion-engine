#pragma once

namespace MotionEngineDSP
{

class TransientDetector
{
public:
    TransientDetector() = default;
    ~TransientDetector() = default;

    void reset();
    bool process(float input);
    void setThreshold(float threshold);
    void setRefractoryPeriod(int samples);

private:
    float threshold = 0.1f;
    int refractoryPeriod = 4410; // 100ms at 44.1kHz
    int refractoryCounter = 0;
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
};

} // namespace MotionEngineDSP