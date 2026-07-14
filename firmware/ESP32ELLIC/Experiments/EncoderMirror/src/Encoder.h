#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

class Encoder
{
public:

    bool begin();

    float readRawAngle();

    float readJointAngle();

    void calibrateZero();

    bool isCalibrated();

private:

    AS5600 sensor;

    float zeroAngle = 0.0f;
    bool calibrated = false;
};