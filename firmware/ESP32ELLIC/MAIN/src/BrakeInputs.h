#pragma once
#include <Arduino.h>
#include "Config.h"
#include "InputFilter.h"

// BrakeInputs: два входа тормозов (LEFT_BRAKE_PIN, RIGHT_BRAKE_PIN) с
// программным фильтром (раздел 4, 8, 13 спецификации).
//
// v3.4: фильтр "уровень не менялся BRAKE_DEBOUNCE_MS" заменён интегрирующим
// (InputFilter). Прежний при постоянной помехе на входе не мог принять
// отпускание тормоза - тормоз залипал, и вал в TURN_ZONE давал повороты.
// update() вызывается на каждом проходе loop(). Тормоз нажат при LOW.
//
// Отброшенные импульсы считаются (glitches), ширина самого длинного за период
// запоминается (glitchMaxMs): это видно в телеметрии (записи E и D).
class BrakeInputs {
public:
    BrakeInputs();

    // Настраивает пины (INPUT_PULLUP) и запоминает начальное состояние.
    // Зажатый при включении тормоз считается нажатым.
    void begin();

    // Вызывать в каждом проходе loop().
    void update();

    bool leftPressed() const  { return _left.stable(); }
    bool rightPressed() const { return _right.stable(); }

    BrakeSnapshot getSnapshot() const;

    // Сбрасывает статистику за период (glitchMaxMs). Вызывает Telemetry после
    // выдачи пакета.
    void resetPeriodStats();

private:
    InputFilter _left;
    InputFilter _right;
};
