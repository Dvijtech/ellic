#pragma once
#include <Arduino.h>

class ODriveUART
{
public:
    ODriveUART(HardwareSerial &serial, const char *name);

    void begin(uint32_t baud, int rxPin, int txPin);

    void sendPosition(float turns);
    void setIdle();
    void setClosedLoop();
    void enable();
    void disable();
    void resetClosedLoopFlag();
    void reinit(float targetTurns, uint32_t bootDelayMs = 1500);

    // Асинхронный запрос одного float-параметра ODrive (не блокирует loop)
    void requestFloat(const char *property);
    // Вызывать каждый loop(); true = запрос завершён (успех или таймаут)
    bool pollFloat(float &out, bool &ok);
    bool isBusy() const { return _state == State::Waiting; }

    bool isAlive() const { return _alive; }
    const char *name() const { return _name; }

private:
    enum class State { Idle, Waiting };
    static constexpr uint32_t REQUEST_TIMEOUT_MS = 50;

    HardwareSerial &_serial;
    const char *_name;
    bool _closedLoop = false;
    bool _alive = false;
    State _state = State::Idle;
    unsigned long _requestStart = 0;
};