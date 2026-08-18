#define ODRIVE_DEBUG_RAW

#include "ODriveUART.h"

ODriveUART::ODriveUART(HardwareSerial &serial, const char *name)
    : _serial(serial), _name(name) {}

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

void ODriveUART::setIdle()       { _serial.println("w axis0.requested_state 1"); }
void ODriveUART::setClosedLoop() { _serial.println("w axis0.requested_state 8"); }

void ODriveUART::enable()  { if (!_closedLoop) { setClosedLoop(); _closedLoop = true; } }
void ODriveUART::disable() { if (_closedLoop)  { setIdle();       _closedLoop = false; } }
void ODriveUART::resetClosedLoopFlag() { _closedLoop = false; }

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

void ODriveUART::requestFloat(const char *property)
{
    _lastProperty = property;

    while (_serial.available()) _serial.read();
    _serial.print("r ");
    _serial.println(property);
    _requestStart = millis();
    _state = State::Waiting;
}

bool ODriveUART::pollFloat(float &out, bool &ok)
{
    if (_state != State::Waiting) return false;

    if (_serial.available())
    {
        String s = _serial.readStringUntil('\n');
        s.trim();

        if (s.length() > 0)
        {
            out = s.toFloat();
            ok = true;
            _alive = true;
            _state = State::Idle;

#ifdef ODRIVE_DEBUG_RAW
            Serial.print(_name); Serial.print(" ");
            Serial.print(_lastProperty);
            Serial.print(" -> raw:'"); Serial.print(s); Serial.println("'");
#endif
            return true;
        }
        return false;
    }

    if (millis() - _requestStart >= REQUEST_TIMEOUT_MS)
    {
        ok = false;
        _alive = false;
        _state = State::Idle;

#ifdef ODRIVE_DEBUG_RAW
        Serial.print(_name); Serial.print(" ");
        Serial.print(_lastProperty);
        Serial.println(" -> TIMEOUT");
#endif
        return true;
    }

    if (ok) {
        _alive = true;
        _failCount = 0;
    } else {
        _failCount++;
        if (_failCount >= ALIVE_FAIL_THRESHOLD)
            _alive = false;
    }

    return false;
}