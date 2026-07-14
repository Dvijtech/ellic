#include "PhaseDetector.h"

#include <math.h>

void PhaseDetector::begin(float zeroAngle)
{
    _zero = zeroAngle;

    _angle = 0;
    _jointAngle = 0;

    _continuous = 0;
    _filtered = 0;

    _lastRaw = 0;

    _moving = false;

    _phase = GaitPhase::STOP;
}

float PhaseDetector::angle()
{
    return _angle;
}

float PhaseDetector::jointAngle()
{
    return _jointAngle;
}

float PhaseDetector::continuousAngle()
{
    return _filtered;
}

ArcSide PhaseDetector::side()
{
    return _side;
}

bool PhaseDetector::moving()
{
    return _moving;
}

GaitPhase PhaseDetector::phase()
{
    return _phase;
}

float PhaseDetector::motorTurns(float gearRatio)
{
    return _filtered * gearRatio / 360.0f;
}

void PhaseDetector::update(float encoderAngle)
{
    //-------------------------------------------------
    // Абсолютный угол 0...360
    //-------------------------------------------------

    _angle = encoderAngle - _zero;

    while (_angle < 0)
        _angle += 360;

    while (_angle >= 360)
        _angle -= 360;

    //-------------------------------------------------
    // Unwrap
    //-------------------------------------------------

    float delta = _angle - _lastRaw;

    if (delta > 180)
        delta -= 360;

    if (delta < -180)
        delta += 360;

    _continuous += delta;
    _lastRaw = _angle;

    //-------------------------------------------------
    // EMA фильтр
    //-------------------------------------------------

    _filtered =
        _filtered * (1.0f - EMA_ALPHA) +
        _continuous * EMA_ALPHA;

    //-------------------------------------------------
    // Движение
    //-------------------------------------------------

    _moving = fabs(delta) > MOVE_THRESHOLD;

    //-------------------------------------------------
    // Текущий цикл 0...360
    //-------------------------------------------------

    float cycle = fmod(_filtered, 360.0f);

    if (cycle < 0)
        cycle += 360;

    //-------------------------------------------------
    // Передняя/задняя дуга
    //-------------------------------------------------

    if (cycle <= 180)
    {
        _jointAngle = cycle;
        _side = ArcSide::FRONT;
    }
    else
    {
        _jointAngle = 360 - cycle;
        _side = ArcSide::BACK;
    }

    //-------------------------------------------------
    // Стоим
    //-------------------------------------------------

    if (!_moving)
    {
        _phase = GaitPhase::STOP;
        return;
    }

    //-------------------------------------------------
    // Фазы шага
    //-------------------------------------------------

    if (_side == ArcSide::FRONT)
    {
        if (_jointAngle < 15)
            _phase = GaitPhase::LEFT_START;
        else if (_jointAngle < 70)
            _phase = GaitPhase::STOP;
        else
            _phase = GaitPhase::LEFT_PUSH;
    }
    else
    {
        if (_jointAngle < 15)
            _phase = GaitPhase::RIGHT_START;
        else if (_jointAngle < 70)
            _phase = GaitPhase::STOP;
        else
            _phase = GaitPhase::RIGHT_PUSH;
    }
}