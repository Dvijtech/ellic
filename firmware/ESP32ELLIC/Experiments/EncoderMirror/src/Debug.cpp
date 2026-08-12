#include "Debug.h"

void Debug::begin(uint32_t printPeriodMs)
{
    _printPeriodMs = printPeriodMs;
    _lastPrint = 0;
}

void Debug::update(const Encoder &encoder,
                    const PhaseDetector &phase,
                    const MotionController &motion)
{
    if (millis() - _lastPrint < _printPeriodMs)
        return;

    _lastPrint = millis();

    Serial.print("RAW:");     Serial.print(encoder.rawAngle(), 2);
    Serial.print(" TARGET:"); Serial.print(motion.targetTurns(), 3);
    Serial.print(" LB:");     Serial.print(phase.leftBrake());
    Serial.print(" RB:");     Serial.print(phase.rightBrake());
    Serial.print(" Z:");      Serial.print(PhaseDetector::inTurnZone(encoder.rawAngle()));
    Serial.print(" LO:");     Serial.print(motion.leftHoldOffset(), 3);
    Serial.print(" RO:");     Serial.println(motion.rightHoldOffset(), 3);
}

void Debug::log(const char *msg)
{
    Serial.println(msg);
}

void Debug::logOdriveReinit(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive REINIT");
}

void Debug::logOdriveReady(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive READY");
}

void Debug::logOdrivePowerRestored(const char *name)
{
    Serial.print(name);
    Serial.println(" ODrive POWER RESTORED");
}