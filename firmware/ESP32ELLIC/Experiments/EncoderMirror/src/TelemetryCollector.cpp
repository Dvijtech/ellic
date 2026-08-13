#include "TelemetryCollector.h"

TelemetryCollector::TelemetryCollector(ODriveUART &left, ODriveUART &right, uint32_t periodMs)
    : _left(left), _right(right), _periodMs(periodMs) {}

void TelemetryCollector::setContext(float rawAngle, float targetTurns,
                                     bool leftBrake, bool rightBrake,
                                     bool turnZone, bool turning)
{
    _sample.rawAngle = rawAngle;
    _sample.targetTurns = targetTurns;
    _sample.leftBrake = leftBrake;
    _sample.rightBrake = rightBrake;
    _sample.turnZone = turnZone;
    _sample.turning = turning;
}

void TelemetryCollector::begin()
{
    _cycleStart = millis();
    startStep(Step::LeftCurrent);
}

void TelemetryCollector::startStep(Step step)
{
    _step = step;
    switch (step)
    {
        case Step::LeftCurrent:   _left.requestFloat("axis0.motor.current_control.Iq_measured");  break;
        case Step::LeftVoltage:   _left.requestFloat("axis0.motor.current_control.Vq_setpoint");   break;
        case Step::LeftVelocity:  _left.requestFloat("axis0.encoder.vel_estimate");                break;
        case Step::RightCurrent:  _right.requestFloat("axis0.motor.current_control.Iq_measured");  break;
        case Step::RightVoltage:  _right.requestFloat("axis0.motor.current_control.Vq_setpoint");  break;
        case Step::RightVelocity: _right.requestFloat("axis0.encoder.vel_estimate");               break;
        default: break;
    }
}

void TelemetryCollector::update()
{
    if (_step == Step::Done)
    {
        if (millis() - _cycleStart < _periodMs) return; // не долбим UART чаще заданной частоты

        _sample.tMs = millis();
        _ready = true;
        _cycleStart = millis();
        startStep(Step::LeftCurrent);
        return;
    }

    bool isLeft = (_step == Step::LeftCurrent || _step == Step::LeftVoltage || _step == Step::LeftVelocity);
    ODriveUART &drive = isLeft ? _left : _right;

    float value; bool ok;
    if (!drive.pollFloat(value, ok)) return; // ещё ждём ответ

    if (ok)
    {
        switch (_step)
        {
            case Step::LeftCurrent:   _sample.leftCurrent   = value; break;
            case Step::LeftVoltage:   _sample.leftVoltage   = value; break;
            case Step::LeftVelocity:  _sample.leftVelocity  = value; break;
            case Step::RightCurrent:  _sample.rightCurrent  = value; break;
            case Step::RightVoltage:  _sample.rightVoltage  = value; break;
            case Step::RightVelocity: _sample.rightVelocity = value; break;
            default: break;
        }
    } // при таймауте (ok=false) поле просто не обновляется

    switch (_step)
    {
        case Step::LeftCurrent:   startStep(Step::LeftVoltage);   break;
        case Step::LeftVoltage:   startStep(Step::LeftVelocity);  break;
        case Step::LeftVelocity:  startStep(Step::RightCurrent);  break;
        case Step::RightCurrent:  startStep(Step::RightVoltage);  break;
        case Step::RightVoltage:  startStep(Step::RightVelocity); break;
        case Step::RightVelocity: _step = Step::Done;             break;
        default: break;
    }
}

bool TelemetryCollector::sampleReady()
{
    if (_ready) { _ready = false; return true; }
    return false;
}