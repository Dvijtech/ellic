#include "ODriveUART.h"

ODriveUART::ODriveUART(
    HardwareSerial *serial,
    float direction)
{
    _serial = serial;
    _direction = direction;
}

/////////////////////////////////////////////////////

void ODriveUART::begin(
    uint8_t rxPin,
    uint8_t txPin)
{
    _serial->begin(
        115200,
        SERIAL_8N1,
        rxPin,
        txPin);

    delay(300);

    closedLoop();
}

/////////////////////////////////////////////////////

void ODriveUART::idle()
{
    _serial->println("w axis0.requested_state 1");
}

/////////////////////////////////////////////////////

void ODriveUART::closedLoop()
{
    _serial->println("w axis0.requested_state 8");
}

/////////////////////////////////////////////////////

void ODriveUART::setPosition(
    float turns)
{
    turns *= _direction;

    _serial->print("p 0 ");
    _serial->println(turns, 4);
}

/////////////////////////////////////////////////////

void ODriveUART::setTorque(
    float torque)
{
    torque *= _direction;

    _serial->print("c 0 ");
    _serial->println(torque, 3);
}