#pragma once
#include <stdint.h>

// InputFilter: интегрирующий фильтр дискретного входа (v3.4).
//
// Зачем. Фильтр v3.3 принимал новое состояние, только если сырой уровень не
// менялся thresholdMs подряд. При постоянных коротких импульсах помехи
// (сотни в секунду) такого "тихого" окна нет вообще: тормоз, однажды принятый
// как нажатый, не отпускался (в логе 04.11 - около 1300 и 570 отброшенных
// импульсов в секунду на левом и правом тормозах).
//
// Как работает. Накопитель score (мкс) растёт, пока сырой уровень "нажат", и
// убывает, пока "отпущен". Состояние становится "нажат", когда score достигает
// thresholdMs, и "отпущен", когда score падает до нуля. Редкие короткие
// импульсы помехи (доля времени ниже 50%) накопитель не сдвигают, а настоящее
// нажатие или отпускание проходит за те же thresholdMs, что и раньше.
//
// Один шаг интегрирования ограничен MAX_STEP_US: пауза loop() не должна
// засчитываться как долгий уровень.
//
// Отброшенным импульсом (glitch) считается отклонение сырого уровня от
// принятого состояния, которое закончилось без смены состояния. Счётчик и
// ширина самого длинного за период идут в телеметрию как раньше.
class InputFilter {
public:
    static const uint32_t MAX_STEP_US = 1000;

    void begin(bool raw, uint32_t nowUs, uint32_t thresholdMs) {
        _thresholdUs = thresholdMs * 1000UL;
        _stable = raw;
        _scoreUs = raw ? _thresholdUs : 0;
        _lastUs = nowUs;
        _inPulse = false;
        _pulseStartUs = 0;
        _glitches = 0;
        _glitchMaxMs = 0;
    }

    // raw: true - вход в активном состоянии (LOW). Возвращает true в тот вызов,
    // в который принятое состояние изменилось.
    bool update(bool raw, uint32_t nowUs) {
        uint32_t dt = nowUs - _lastUs;
        _lastUs = nowUs;
        if (dt > MAX_STEP_US) dt = MAX_STEP_US;

        if (raw != _stable) {
            if (!_inPulse) { _inPulse = true; _pulseStartUs = nowUs; }
        } else if (_inPulse) {
            _inPulse = false;
            _glitches++;
            uint32_t widthMs = (nowUs - _pulseStartUs) / 1000UL;
            if (widthMs > _glitchMaxMs) _glitchMaxMs = widthMs;
        }

        if (raw) {
            _scoreUs += dt;
            if (_scoreUs > _thresholdUs) _scoreUs = _thresholdUs;
        } else {
            _scoreUs = (_scoreUs > dt) ? (_scoreUs - dt) : 0;
        }

        bool changed = false;
        if (!_stable && _scoreUs >= _thresholdUs) {
            _stable = true;
            changed = true;
        } else if (_stable && _scoreUs == 0) {
            _stable = false;
            changed = true;
        }
        if (changed) _inPulse = false;   // настоящая смена, а не импульс помехи
        return changed;
    }

    bool     stable() const      { return _stable; }
    uint32_t glitches() const    { return _glitches; }
    uint32_t glitchMaxMs() const { return _glitchMaxMs; }
    void     resetPeriodStats()  { _glitchMaxMs = 0; }

private:
    uint32_t _thresholdUs = 0;
    uint32_t _scoreUs = 0;
    uint32_t _lastUs = 0;
    uint32_t _pulseStartUs = 0;
    uint32_t _glitches = 0;
    uint32_t _glitchMaxMs = 0;
    bool     _stable = false;
    bool     _inPulse = false;
};
