#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Config.h"

class Telemetry;

// Encoder: чтение AS5600, накопление continuousAngle (раздел 6 спецификации).
//
// v3.2: AS5600 опрашивается в ОТДЕЛЬНОЙ задаче FreeRTOS на ядре 0 (как BLE),
// раз в ENCODER_PERIOD_MS. Причина: транзакция I2C с датчиком иногда зависала
// ровно на ~1 с (по логам), и пока она не вернулась, весь loop() стоял - не
// принимался CAN и не уходили команды колёсам. Теперь зависание I2C задерживает
// только саму задачу энкодера; loop() только читает последний результат через
// getSnapshot() и никогда не обращается к Wire.
//
// После ошибки чтения шина I2C восстанавливается (9 импульсов SCL, STOP,
// повторная инициализация Wire). Если от последнего успешного чтения прошло
// больше ENCODER_RESYNC_MS, первое успешное чтение только перепривязывает
// точку отсчёта (дельта не считается) - см. раздел 6.3 спецификации.
class Encoder {
public:
    Encoder();

    // telemetry может быть nullptr. Запускает задачу чтения датчика.
    void begin(Telemetry* telemetry);

    // Последний опубликованный результат (быстро, без обращения к I2C).
    EncoderSnapshot getSnapshot() const;

    // Сбрасывает статистику за период (readMaxUs). Вызывает Telemetry после
    // выдачи пакета.
    void resetPeriodStats();

private:
    static void taskEntry(void* param);
    void taskLoop();

    bool readRawAngleDeg(float &outDeg);
    void recoverBus();
    static void busClear();

    Telemetry* _telemetry;
    TaskHandle_t _task;
    mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

    // Принадлежит задаче (из loop() не читается и не пишется):
    bool     _initialized;
    float    _previousRawAngle;  // последнее валидное сырое значение, град.
    float    _continuousAngle;   // накопленный угол, град.
    float    _lastDelta;         // последняя посчитанная дельта
    bool     _hasOk;             // было ли хотя бы одно успешное чтение
    uint32_t _lastOkMs;          // момент последнего успешного чтения
    uint32_t _failStreak;        // подряд неудачных чтений

    // Опубликовано задачей, читается из loop() под защитой _mux:
    float    _pubRaw;
    float    _pubCont;
    float    _pubDelta;
    uint32_t _pubErrors;         // накопительно: неудачные чтения
    uint32_t _pubReadMaxUs;      // максимум длительности чтения за период
    uint32_t _pubRecoveries;     // накопительно: восстановления шины I2C
};
