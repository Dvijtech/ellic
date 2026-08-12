#include "MotionController.h"

MotionController::MotionController(Encoder &encoder,
                                    ODriveUART &leftDrive,
                                    ODriveUART &rightDrive,
                                    PhaseDetector &phase,
                                    float gearRatio,
                                    float turnStep,
                                    uint32_t controlPeriodMs)
    : _encoder(encoder), _left(leftDrive), _right(rightDrive), _phase(phase),
      _gearRatio(gearRatio), _turnStep(turnStep), _controlPeriodMs(controlPeriodMs)
{
}

void MotionController::begin()
{
    _left.enable();
    _right.enable();
}

void MotionController::finishTurn()
{
    _leftHoldOffsetW += _leftTurnOffsetW;
    _rightHoldOffsetW += _rightTurnOffsetW;

    _leftTurnOffsetW = 0;
    _rightTurnOffsetW = 0;

    _turning = false;
}

void MotionController::update()
{
    _targetTurnsV = -_encoder.continuousAngle() * _gearRatio / 360.0f;

    if (millis() - _lastControl < _controlPeriodMs)
        return;

    _lastControl = millis();

    bool leftBrake = _phase.leftBrake();
    bool rightBrake = _phase.rightBrake();
    bool zone = PhaseDetector::inTurnZone(_encoder.rawAngle());

    if (leftBrake && rightBrake)
    {
        _left.disable();
        _right.disable();
        _turning = true;
    }
    else if (leftBrake)
    {
        if (zone)
        {
            _turning = true;
            _left.disable();
            _right.enable();

            _rightTurnOffsetW -= _turnStep;
            float rightTarget = -_targetTurnsV + _rightHoldOffsetW + _rightTurnOffsetW;
            _right.sendPosition(rightTarget);
        }
        else
        {
            _left.disable();
            _right.disable();
        }
    }
    else if (rightBrake)
    {
        if (zone)
        {
            _turning = true;
            _right.disable();
            _left.enable();

            _leftTurnOffsetW += _turnStep;
            float leftTarget = _targetTurnsV + _leftHoldOffsetW + _leftTurnOffsetW;
            _left.sendPosition(leftTarget);
        }
        else
        {
            _left.disable();
            _right.disable();
        }
    }
    else
    {
        if (_turning)
            finishTurn();

        _left.enable();
        _right.enable();

        _left.sendPosition(_targetTurnsV + _leftHoldOffsetW);
        _right.sendPosition(-_targetTurnsV + _rightHoldOffsetW);
    }
}