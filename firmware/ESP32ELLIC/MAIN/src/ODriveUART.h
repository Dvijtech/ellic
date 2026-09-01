#pragma once

#include <Arduino.h>

class ODriveUART
{
public:
    ODriveUART(
        HardwareSerial &serial,
        const char *name
    );

    void begin(
        uint32_t baud,
        int rxPin,
        int txPin
    );

    void configure();

    void enable();
    void disable();

    bool getPosition(
        float &position
    );

    bool getPositionVelocity(
        float &position,
        float &velocity
    );

    void sendPosition(
        float motorTurns
    );

    bool isAlive() const
    {
        return _alive;
    }

    const char *name() const
    {
        return _name;
    }

private:

    HardwareSerial &_serial;
    const char *_name;

    bool _alive = false;
    bool _closedLoop = false;

    static constexpr uint32_t RESPONSE_TIMEOUT_MS = 50;

    static constexpr float ODRIVE_VEL_LIMIT = 2.0f;
    static constexpr float ODRIVE_ACCEL_LIMIT = 10.0f;

    void flushInput();

    bool readLine(
        String &line,
        uint32_t timeoutMs
    );

    bool readPositionVelocity(
        float &position,
        float &velocity
    );
};