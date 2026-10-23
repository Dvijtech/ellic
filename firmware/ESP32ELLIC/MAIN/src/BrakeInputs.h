#pragma once
#include <Arduino.h>
#include "Config.h"

// BrakeInputs: два входа тормозов (LEFT_BRAKE_PIN, RIGHT_BRAKE_PIN) с
// программным фильтром (раздел 4, 8, 13 спецификации).
//
// Раньше loop() читал тормоза раз в CONTROL_PERIOD_MS одним digitalRead() без
// фильтрации: любая помеха на длинном проводе в момент чтения превращалась в
// «нажатый тормоз» на весь цикл. Теперь update() вызывается на каждом проходе
// loop(), а состояние тормоза меняется, только когда сырой уровень не менялся
// не менее BRAKE_DEBOUNCE_MS. Тормоз нажат при LOW (подтяжка к 3.3 В).
//
// Импульсы, не дожившие до BRAKE_DEBOUNCE_MS, отбрасываются и считаются
// (glitches), а ширина самого длинного за период запоминается (glitchMaxMs):
// это видно в телеметрии и показывает, какой помехой это было.
class BrakeInputs {
public:
    BrakeInputs();

    // Настраивает пины (INPUT_PULLUP) и запоминает начальное состояние.
    // Зажатый при включении тормоз считается нажатым.
    void begin();

    // Вызывать в каждом проходе loop().
    void update();

    bool leftPressed() const  { return _left.stable; }
    bool rightPressed() const { return _right.stable; }

    BrakeSnapshot getSnapshot() const;

    // Сбрасывает статистику за период (glitchMaxMs). Вызывает Telemetry после
    // выдачи пакета.
    void resetPeriodStats();

private:
    struct Channel {
        int      pin;
        bool     stable;        // состояние после фильтра: true - тормоз нажат
        bool     lastRaw;       // последний сырой уровень (true - LOW)
        uint32_t changedMs;     // когда сырой уровень менялся в последний раз
        uint32_t glitches;      // накопительно: отброшенные импульсы
        uint32_t glitchMaxMs;   // макс. ширина отброшенного импульса за период
    };

    static void initChannel(Channel &ch, int pin);
    static void updateChannel(Channel &ch, uint32_t nowMs);

    Channel _left;
    Channel _right;
};
