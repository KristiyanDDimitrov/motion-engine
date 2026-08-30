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

        void setType(DriveType type);
        void setAmount(float amount);
        void setMix(float mix);
        float process(float input);

    private:
        DriveType type = DriveType::softClip;
        float amount = 0.0f;
        float mix = 0.0f;
    };
}