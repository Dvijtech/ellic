#include "Encoder.h"

bool Encoder::begin()
{
    Wire.begin(21, 22);
    Wire.setClock(100000);

    delay(200);

    if (!sensor.begin())
    {
        Serial.println("AS5600 ERROR");
        return false;
    }

    if (!sensor.magnetDetected())
    {
        Serial.println("MAGNET ERROR");
        return false;
    }

    Serial.println("AS5600 OK");

    return true;
}

float Encoder::readRawAngle()
{
    uint16_t raw = sensor.readAngle();

    return raw * 360.0f / 4096.0f;
}

float Encoder::readJointAngle()
{
    if (!calibrated)
        return 0.0f;

    float angle = readRawAngle() - zeroAngle;

    while (angle < 0)
        angle += 360;

    while (angle >= 360)
        angle -= 360;

    return angle;
}

void Encoder::calibrateZero()
{
    zeroAngle = readRawAngle();

    calibrated = true;

    Serial.print("ZERO CALIBRATED: ");
    Serial.println(zeroAngle, 2);
}

bool Encoder::isCalibrated()
{
    return calibrated;
}