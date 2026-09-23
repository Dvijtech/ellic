#pragma once
#include <Arduino.h>
#include "Config.h"

class Encoder;
class MotionController;
class ODriveCAN;
class BleTelemetry;

// Telemetry: единственный модуль, отвечающий за вывод телеметрии.
// PULL-путь: collect() раз в TELEMETRY_PERIOD_MS, printScheduled() раз в
// TELEMETRY_PRINT_PERIOD_MS (раздел 12.1).
// PUSH-путь: log() вызывается любым модулем в момент события (раздел 12.2).
//
// Канал вывода не привязан к Serial жёстко: весь текст проходит через
// emit() (см. Telemetry.cpp), который пишет в Serial и, если подключён
// BleTelemetry, дублирует тот же текст в BLE - состав и формат телеметрии
// в обоих каналах гарантированно одинаковы, т.к. формируются один раз.
class Telemetry {
public:
    Telemetry(Encoder* encoder, MotionController* motion, ODriveCAN* odriveCAN);

    void begin(LogLevel minLevel = LogLevel::INFO);

    // Опционально подключить BLE-канал. По умолчанию nullptr - тогда
    // поведение полностью совпадает с прежним (вывод только в Serial).
    void setBleTelemetry(BleTelemetry* ble);

    // Вызывать в каждом проходе loop().
    void update();

    // PUSH-путь. ERROR/CRITICAL печатаются немедленно и синхронно;
    // INFO/WARNING складываются в кольцевой буфер до ближайшего printScheduled().
    void log(LogLevel level, const char* module, const char* msg);

private:
    void collect();
    void printScheduled();
    void printSample() const;
    void printLogBuffer();

    // Единая точка вывода готовой строки: Serial + (опционально) BLE.
    void emit(const char* text) const;

    Encoder* _encoder;
    MotionController* _motion;
    ODriveCAN* _odriveCAN;
    BleTelemetry* _ble;

    TelemetrySample _sample;
    uint32_t _lastCollectMs;
    uint32_t _lastPrintMs;
    LogLevel _minLevel;

    struct LogEntry {
        LogLevel level;
        char module[16];
        char msg[80];
    };
    static const int LOG_BUFFER_SIZE = 16;
    LogEntry _logBuffer[LOG_BUFFER_SIZE];
    int _logWriteIndex;
    int _logCount;
};
