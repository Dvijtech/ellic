#include "ODriveUART.h"

ODriveUART::ODriveUART(
    HardwareSerial &serial,
    const char *name
)
    : _serial(serial),
      _name(name)
{
}

void ODriveUART::begin(
    uint32_t baud,
    int rxPin,
    int txPin
)
{
    _serial.begin(
        baud,
        SERIAL_8N1,
        rxPin,
        txPin
    );
}

void ODriveUART::configure()
{
    flushInput();

    _serial.println(
        "w axis0.controller.config.control_mode 3"
    );

    delay(20);

    _serial.println(
        "w axis0.controller.config.input_mode 2"
    );

    delay(20);

    _serial.printf(
        "w axis0.trap_traj.config.vel_limit %.4f\n",
        ODRIVE_VEL_LIMIT
    );

    delay(20);

    _serial.printf(
        "w axis0.trap_traj.config.accel_limit %.4f\n",
        ODRIVE_ACCEL_LIMIT
    );

    delay(20);

    _serial.printf(
        "w axis0.trap_traj.config.decel_limit %.4f\n",
        ODRIVE_ACCEL_LIMIT
    );

    delay(20);

    _serial.println(
        "w axis0.requested_state 8"
    );

    delay(100);

    _closedLoop = true;
    _alive = true;
}

void ODriveUART::enable()
{
    if (_closedLoop)
        return;

    flushInput();

    _serial.println(
        "w axis0.requested_state 8"
    );

    delay(50);

    _closedLoop = true;
}

void ODriveUART::disable()
{
    if (!_closedLoop)
        return;

    flushInput();

    _serial.println(
        "w axis0.requested_state 1"
    );

    delay(50);

    _closedLoop = false;
}

bool ODriveUART::getPosition(
    float &position
)
{
    float velocity;

    return getPositionVelocity(
        position,
        velocity
    );
}

bool ODriveUART::getPositionVelocity(
    float &position,
    float &velocity
)
{
    flushInput();

    _serial.println("f 0");

    return readPositionVelocity(
        position,
        velocity
    );
}

void ODriveUART::sendPosition(
    float motorTurns
)
{
    _serial.printf(
        "p 0 %.4f\n",
        motorTurns
    );
}

void ODriveUART::flushInput()
{
    while (_serial.available())
    {
        _serial.read();
    }
}

bool ODriveUART::readLine(
    String &line,
    uint32_t timeoutMs
)
{
    uint32_t start = millis();

    line = "";

    while (
        millis() - start <
        timeoutMs
    )
    {
        while (_serial.available())
        {
            char c = _serial.read();

            if (c == '\n')
            {
                line.trim();

                return line.length() > 0;
            }

            if (c != '\r')
            {
                line += c;
            }
        }

        delay(1);
    }

    return false;
}

bool ODriveUART::readPositionVelocity(
    float &position,
    float &velocity
)
{
    String line;

    if (!readLine(
        line,
        RESPONSE_TIMEOUT_MS
    ))
    {
        _alive = false;
        return false;
    }

    int separator = line.indexOf(' ');

    if (separator < 0)
    {
        _alive = false;
        return false;
    }

    String positionString =
        line.substring(
            0,
            separator
        );

    String velocityString =
        line.substring(
            separator + 1
        );

    position = positionString.toFloat();
    velocity = velocityString.toFloat();

    _alive = true;

    return true;
}