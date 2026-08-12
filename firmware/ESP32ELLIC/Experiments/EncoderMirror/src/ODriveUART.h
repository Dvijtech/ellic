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

    // Пингует привод, true если ответил
    bool checkAlive();

    void enable();
    void disable();

    // Сбросить флаги при потере связи (не шлёт команды, просто локальное состояние)
    void markDisconnected();

    // Полная процедура восстановления после пропадания питания
    void reinit(float targetTurns, uint32_t bootDelayMs = 1500);

    bool isClosedLoop() const { return _closedLoop; }
    bool isAlive()       const { return _alive; }
    const char *name()   const { return _name; }

private:
    HardwareSerial &_serial;
    const char *_name;

    bool _closedLoop = false;
    bool _alive = false;
};