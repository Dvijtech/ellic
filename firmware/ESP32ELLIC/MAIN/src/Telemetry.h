#pragma once
#include <Arduino.h>
#include "Config.h"

class Encoder;
class MotionController;
class ODriveUART;

// Telemetry: единственный модуль, отвечающий за вывод в Serial.
// PULL-путь: collect() раз в TELEMETRY_PERIOD_MS, printScheduled() раз в
// TELEMETRY_PRINT_PERIOD_MS (раздел 12.1).
// PUSH-путь: log() вызывается любым модулем в момент события (раздел 12.2).
class Telemetry {
public:
    Telemetry(Encoder* encoder, MotionController* motion,
              ODriveUART* left, ODriveUART* right);

    void begin(LogLevel minLevel = LogLevel::INFO);

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

    Encoder* _encoder;
    MotionController* _motion;
    ODriveUART* _left;
    ODriveUART* _right;

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
    int _logWriteIndex; // следующая позиция записи (0..LOG_BUFFER_SIZE-1)
    int _logCount;      // сколько записей ждут ближайшего printScheduled() (<= LOG_BUFFER_SIZE)
};
