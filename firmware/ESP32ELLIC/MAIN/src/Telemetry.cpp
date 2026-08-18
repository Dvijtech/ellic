#include "Telemetry.h"

// ============================================================
// CONSTRUCTOR
// ============================================================

Telemetry::Telemetry(
    ODriveUART &left,
    ODriveUART &right,
    uint32_t periodMs,
    uint32_t printPeriodMs)
    : _left(left),
      _right(right),
      _periodMs(periodMs),
      _printPeriodMs(printPeriodMs)
{
}

// ============================================================
// BEGIN
// ============================================================

void Telemetry::begin()
{
    _cycleStart = millis();
    _lastPrint = millis();

    _step = Step::LeftCurrent;

    startStep(Step::LeftCurrent);
}

// ============================================================
// SET CONTEXT
// ============================================================

void Telemetry::setContext(
    float rawAngle,
    float targetTurns,
    bool leftBrake,
    bool rightBrake,
    bool turnZone,
    bool turning,
    float leftHoldOffset,
    float rightHoldOffset)
{
    _sample.rawAngle = rawAngle;
    _sample.targetTurns = targetTurns;

    _sample.leftBrake = leftBrake;
    _sample.rightBrake = rightBrake;

    _sample.turnZone = turnZone;
    _sample.turning = turning;

    _sample.leftHoldOffset = leftHoldOffset;
    _sample.rightHoldOffset = rightHoldOffset;
}

// ============================================================
// START ODRIVE REQUEST
// ============================================================

void Telemetry::startStep(Step step)
{
    _step = step;

    switch (step)
    {
        case Step::LeftCurrent:
            _left.requestFloat(
                "axis0.motor.current_control.Iq_measured");
            break;

        case Step::LeftVoltage:
            _left.requestFloat(
                "axis0.motor.current_control.Vq_setpoint");
            break;

        case Step::LeftVelocity:
            _left.requestFloat(
                "axis0.encoder.vel_estimate");
            break;

        case Step::RightCurrent:
            _right.requestFloat(
                "axis0.motor.current_control.Iq_measured");
            break;

        case Step::RightVoltage:
            _right.requestFloat(
                "axis0.motor.current_control.Vq_setpoint");
            break;

        case Step::RightVelocity:
            _right.requestFloat(
                "axis0.encoder.vel_estimate");
            break;

        case Step::Done:
            break;
    }
}

// ============================================================
// UPDATE
// ============================================================

void Telemetry::update()
{
    // --------------------------------------------------------
    // PERIODIC SERIAL DEBUG
    // --------------------------------------------------------

    printDebug();

    // --------------------------------------------------------
    // COMPLETE CYCLE
    // --------------------------------------------------------

    if (_step == Step::Done)
    {
        if (millis() - _cycleStart < _periodMs)
            return;

        _sample.tMs = millis();

        _ready = true;

        _cycleStart = millis();

        startStep(Step::LeftCurrent);

        return;
    }

    // --------------------------------------------------------
    // DETERMINE ACTIVE ODRIVE
    // --------------------------------------------------------

    bool isLeft =
        (_step == Step::LeftCurrent ||
         _step == Step::LeftVoltage ||
         _step == Step::LeftVelocity);

    ODriveUART &drive =
        isLeft ? _left : _right;

    // --------------------------------------------------------
    // WAIT FOR RESPONSE
    // --------------------------------------------------------

    float value;
    bool ok;

    if (!drive.pollFloat(value, ok))
        return;

    // --------------------------------------------------------
    // SAVE VALUE
    // --------------------------------------------------------

    if (ok)
    {
        switch (_step)
        {
            case Step::LeftCurrent:
                _sample.leftCurrent = value;
                break;

            case Step::LeftVoltage:
                _sample.leftVoltage = value;
                break;

            case Step::LeftVelocity:
                _sample.leftVelocity = value;
                break;

            case Step::RightCurrent:
                _sample.rightCurrent = value;
                break;

            case Step::RightVoltage:
                _sample.rightVoltage = value;
                break;

            case Step::RightVelocity:
                _sample.rightVelocity = value;
                break;

            case Step::Done:
                break;
        }
    }

    // --------------------------------------------------------
    // NEXT REQUEST
    // --------------------------------------------------------

    switch (_step)
    {
        case Step::LeftCurrent:
            startStep(Step::LeftVoltage);
            break;

        case Step::LeftVoltage:
            startStep(Step::LeftVelocity);
            break;

        case Step::LeftVelocity:
            startStep(Step::RightCurrent);
            break;

        case Step::RightCurrent:
            startStep(Step::RightVoltage);
            break;

        case Step::RightVoltage:
            startStep(Step::RightVelocity);
            break;

        case Step::RightVelocity:
            _step = Step::Done;
            break;

        case Step::Done:
            break;
    }
}

// ============================================================
// SAMPLE READY
// ============================================================

bool Telemetry::sampleReady()
{
    if (_ready)
    {
        _ready = false;
        return true;
    }

    return false;
}

// ============================================================
// SERIAL DEBUG
// ============================================================

void Telemetry::printDebug()
{
    if (millis() - _lastPrint < _printPeriodMs)
        return;

    _lastPrint = millis();

    Serial.print("RAW:");    Serial.print(_sample.rawAngle, 2);
    Serial.print(" TGT:");   Serial.print(_sample.targetTurns, 3);

    Serial.print(" LI:");    Serial.print(_sample.leftCurrent, 2);
    Serial.print(" LV:");    Serial.print(_sample.leftVoltage, 2);
    Serial.print(" LS:");    Serial.print(_sample.leftVelocity, 2);

    Serial.print(" RI:");    Serial.print(_sample.rightCurrent, 2);
    Serial.print(" RV:");    Serial.print(_sample.rightVoltage, 2);
    Serial.print(" RS:");    Serial.print(_sample.rightVelocity, 2);

    Serial.print(" LO:");    Serial.print(_sample.leftHoldOffset, 3);
    Serial.print(" RO:");    Serial.print(_sample.rightHoldOffset, 3);

    Serial.print(" LB:");    Serial.print(_sample.leftBrake);
    Serial.print(" RB:");    Serial.print(_sample.rightBrake);
    Serial.print(" Z:");     Serial.print(_sample.turnZone);
    Serial.print(" TRN:");   Serial.println(_sample.turning);
}

// ============================================================
// ODRIVE LOGGING
// ============================================================

void Telemetry::logOdriveReinit(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive REINIT");
}

void Telemetry::logOdriveReady(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive READY");
}

void Telemetry::logOdrivePowerRestored(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive POWER RESTORED");
}