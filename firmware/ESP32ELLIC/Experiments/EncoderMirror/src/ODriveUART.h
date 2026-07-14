#pragma once

#include <Arduino.h>

class ODriveUART
{
public:

    ODriveUART(
        HardwareSerial *serial,
        float direction = 1.0f
    );

    void begin(
        uint8_t rxPin,
        uint8_t txPin
    );

    //--------------------------

    void idle();

    void closedLoop();

    //--------------------------

    void setPosition(
        float turns
    );

    void setTorque(
        float torque
    );

private:

    HardwareSerial *_serial;

    float _direction;
};