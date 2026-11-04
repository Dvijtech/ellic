#pragma once
#include <Arduino.h>
#include "Config.h"
#include "InputFilter.h"

// FireButton: кнопка стрельбы (раздел 4, 13 спецификации).
// Вход FIRE_BUTTON_PIN с INPUT_PULLUP, кнопка нажата при LOW. v3.4: фильтр
// интегрирующий (InputFilter, порог FIRE_DEBOUNCE_MS), как у тормозов.
// Считает нажатия по фронту (отпускание не считается).
// Модуль ничего не отправляет и не управляет моторами: о смене состояния
// сообщает возвращаемым значением update(), а данные отдаёт через getSnapshot().
class FireButton {
public:
    FireButton();

    // Настраивает пин и запоминает начальное состояние (если кнопка зажата при
    // старте, это не считается нажатием).
    void begin();

    // Вызывать в каждом проходе loop(). Возвращает true в тот проход, в
    // который принятое состояние изменилось (нажатие или отпускание).
    bool update();

    FireSnapshot getSnapshot() const;

private:
    InputFilter _filter;
    uint32_t    _pressCount;
};
