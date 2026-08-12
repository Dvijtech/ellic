#include "ODriveUART.h"

ODriveUART::ODriveUART(HardwareSerial &serial, const char *name)
    : _serial(serial), _name(name)
{
}

void ODriveUART::begin(uint32_t baud, int rxPin, int txPin)
{
    _serial.begin(baud, SERIAL_8N1, rxPin, txPin);
}

void ODriveUART::sendPosition(float turns)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "w axis0.controller.input_pos %.4f", turns);
    _serial.println(buffer);
}

void ODriveUART::setIdle()
{
    _serial.println("w axis0.requested_state 1");
}

void ODriveUART::setClosedLoop()
{
    _serial.println("w axis0.requested_state 8");
}

bool ODriveUART::checkAlive()
{
    while (_serial.available())
        _serial.read();

    _serial.println("r vbus_voltage");

    unsigned long start = millis();

    while (millis() - start < 100)
    {
        if (_serial.available())
        {
            String s = _serial.readStringUntil('\n');
            s.trim();

            if (s.length() > 0)
            {
                _alive = true;
                return true;
            }
        }
    }

    _alive = false;
    return false;
}

void ODriveUART::enable()
{
    if (!_closedLoop)
    {
        setClosedLoop();
        _closedLoop = true;
    }
}

void ODriveUART::disable()
{
    if (_closedLoop)
    {
        setIdle();
        _closedLoop = false;
    }
}

void ODriveUART::markDisconnected()
{
    _closedLoop = false;
    _alive = false;
}

void ODriveUART::reinit(float targetTurns, uint32_t bootDelayMs)
{
    _closedLoop = false;

    delay(bootDelayMs);

    sendPosition(targetTurns);
    delay(50);

    setClosedLoop();
    delay(100);

    _closedLoop = true;
}