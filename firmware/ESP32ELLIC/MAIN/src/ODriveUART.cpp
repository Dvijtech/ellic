#include "ODriveUART.h"

#define ODRIVE_DEBUG_RAW

ODriveUART::ODriveUART(
    HardwareSerial &serial,
    const char *name)
    : _serial(serial),
      _name(name)
{
}

// ============================================================
// BEGIN
// ============================================================

void ODriveUART::begin(
    uint32_t baud,
    int rxPin,
    int txPin)
{
    _serial.begin(
        baud,
        SERIAL_8N1,
        rxPin,
        txPin);
}

// ============================================================
// POSITION
// ============================================================

void ODriveUART::sendPosition(float motorTurns)
{
    char buffer[64];

    snprintf(
        buffer,
        sizeof(buffer),
        "w axis0.controller.input_pos %.4f",
        motorTurns);

    _serial.println(buffer);
}

// ============================================================
// ODRIVE STATE
// ============================================================

void ODriveUART::setIdle()
{
    _serial.println(
        "w axis0.requested_state 1");
}

void ODriveUART::setClosedLoop()
{
    _serial.println(
        "w axis0.requested_state 8");
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

void ODriveUART::resetClosedLoopFlag()
{
    _closedLoop = false;
}

// ============================================================
// REINIT
// ============================================================

void ODriveUART::reinit(
    float motorTurns,
    uint32_t bootDelayMs)
{
    _closedLoop = false;

    delay(bootDelayMs);

    sendPosition(motorTurns);

    delay(50);

    setClosedLoop();

    delay(100);

    _closedLoop = true;
}

// ============================================================
// TELEMETRY REQUEST
// ============================================================

void ODriveUART::requestFloat(
    const char *property)
{
    _lastProperty = property;

    while (_serial.available())
        _serial.read();

    _serial.print("r ");
    _serial.println(property);

    _requestStart = millis();

    _state = State::Waiting;
}

// ============================================================
// TELEMETRY POLL
// ============================================================

bool ODriveUART::pollFloat(
    float &out,
    bool &ok)
{
    if (_state != State::Waiting)
        return false;

    // --------------------------------------------------------
    // RESPONSE RECEIVED
    // --------------------------------------------------------

    if (_serial.available())
    {
        String s =
            _serial.readStringUntil('\n');

        s.trim();

        if (s.length() > 0)
        {
            out = s.toFloat();

            ok = true;

            _alive = true;
            _failCount = 0;

            _state = State::Idle;

#ifdef ODRIVE_DEBUG_RAW

            Serial.print(_name);
            Serial.print(" ");
            Serial.print(_lastProperty);
            Serial.print(" -> raw:'");
            Serial.print(s);
            Serial.println("'");

#endif

            return true;
        }

        return false;
    }

    // --------------------------------------------------------
    // TIMEOUT
    // --------------------------------------------------------

    if (millis() - _requestStart >=
        REQUEST_TIMEOUT_MS)
    {
        ok = false;

        _failCount++;

        if (_failCount >=
            ALIVE_FAIL_THRESHOLD)
        {
            _alive = false;
        }

        _state = State::Idle;

#ifdef ODRIVE_DEBUG_RAW

        Serial.print(_name);
        Serial.print(" ");
        Serial.print(_lastProperty);
        Serial.println(" -> TIMEOUT");

#endif

        return true;
    }

    return false;
}