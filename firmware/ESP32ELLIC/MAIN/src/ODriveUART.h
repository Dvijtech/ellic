#pragma once

#include <Arduino.h>

class ODriveUART
{
public:
    ODriveUART(HardwareSerial &serial, const char *name);

    void begin(uint32_t baud, int rxPin, int txPin);

    // --------------------------------------------------------
    // POSITION CONTROL
    // --------------------------------------------------------

    // Передать абсолютную целевую позицию мотора в оборотах.
    void sendPosition(float motorTurns);

    // --------------------------------------------------------
    // STATE
    // --------------------------------------------------------

    // Пока оставляем эти методы для аварийных/служебных случаев.
    // MotionController не должен использовать их
    // как обычный механизм управления движением.
    void setIdle();
    void setClosedLoop();

    void enable();
    void disable();

    // Сброс внутреннего флага после потери ODrive.
    void resetClosedLoopFlag();

    // Восстановление ODrive после перезапуска.
    void reinit(
        float motorTurns,
        uint32_t bootDelayMs = 1500);

    // --------------------------------------------------------
    // TELEMETRY
    // --------------------------------------------------------

    void requestFloat(const char *property);

    bool pollFloat(
        float &out,
        bool &ok);

    bool isBusy() const
    {
        return _state == State::Waiting;
    }

    // --------------------------------------------------------
    // STATUS
    // --------------------------------------------------------

    bool isAlive() const
    {
        return _alive;
    }

    const char *name() const
    {
        return _name;
    }

private:

    const char *_lastProperty = nullptr;

    enum class State
    {
        Idle,
        Waiting
    };

    static constexpr uint32_t REQUEST_TIMEOUT_MS = 50;

    HardwareSerial &_serial;
    const char *_name;

    bool _closedLoop = false;
    bool _alive = false;

    State _state = State::Idle;

    uint32_t _requestStart = 0;

    uint8_t _failCount = 0;

    static constexpr uint8_t ALIVE_FAIL_THRESHOLD = 5;
};