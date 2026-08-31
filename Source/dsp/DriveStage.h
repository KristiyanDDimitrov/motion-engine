#pragma once

namespace MotionEngineDSP
{

enum class DriveType
{
    softClip,
    hardClip,
    wavefold
};

class DriveStage
{
public:
    DriveStage() = default;
    ~DriveStage() = default;

    void reset();
    float process(float input);
    void setAmount(float amount);
    void setType(DriveType type);
    void setMix(float mix); // 0.0 = bypass, 1.0 = full drive

private:
    float amount = 0.5f;
    DriveType type = DriveType::softClip;
    float mix = 0.0f; // Default to bypass

    // State for wavefolder
    float lastInput = 0.0f;
};

} // namespace MotionEngineDSP