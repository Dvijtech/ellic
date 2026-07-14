#pragma once

enum class ArcSide
{
    FRONT,
    BACK
};

enum class GaitPhase
{
    STOP,

    LEFT_START,
    LEFT_PUSH,

    RIGHT_START,
    RIGHT_PUSH
};

class PhaseDetector
{
public:

    void begin(float zeroAngle);

    void update(float encoderAngle);

    float angle();
    float jointAngle();
    float continuousAngle();

    ArcSide side();
    GaitPhase phase();

    bool moving();

    float motorTurns(float gearRatio);

private:

    float _zero = 0;

    float _angle = 0;
    float _jointAngle = 0;

    float _continuous = 0;
    float _filtered = 0;

    float _lastRaw = 0;

    bool _moving = false;

    ArcSide _side = ArcSide::FRONT;
    GaitPhase _phase = GaitPhase::STOP;

    static constexpr float EMA_ALPHA = 0.15f;
    static constexpr float MOVE_THRESHOLD = 2.0f;
};