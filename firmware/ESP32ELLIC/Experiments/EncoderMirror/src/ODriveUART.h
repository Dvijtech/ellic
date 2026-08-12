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

    // Неблокирующая проверка связи, разбита на два шага:
    // 1) requestPing() — отправить запрос
    // 2) pollPing() — вызывать в каждой итерации loop(), пока isPinging() == true
    //    возвращает true один раз, в момент завершения проверки (получен ответ или истёк таймаут)
    void requestPing();
    bool pollPing();

    bool isPinging() const { return _pingState == PingState::Waiting; }

    void enable();
    void disable();

    // Сбросить локальный флаг closedLoop при потере связи (не шлёт команды).
    // _alive уже обновляется внутри pollPing() — здесь его трогать не нужно.
    void resetClosedLoopFlag();

    // Полная процедура восстановления после пропадания питания
    void reinit(float targetTurns, uint32_t bootDelayMs = 1500);

    bool isClosedLoop() const { return _closedLoop; }
    bool isAlive()       const { return _alive; }
    const char *name()   const { return _name; }

private:
    enum class PingState { Idle, Waiting };

    static constexpr uint32_t PING_TIMEOUT_MS = 100;

    PingState _pingState = PingState::Idle;
    unsigned long _pingStart = 0;    

    HardwareSerial &_serial;
    const char *_name;

    bool _closedLoop = false;
    bool _alive = false;
};