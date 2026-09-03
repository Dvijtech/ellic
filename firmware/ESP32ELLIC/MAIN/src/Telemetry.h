#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveUART.h"

enum class LogLevel : uint8_t { INFO = 0, WARNING = 1, ERROR = 2, CRITICAL = 3 };

struct TelemetrySample {
    EncoderSnapshot encoder;
    MotionSnapshot  motion;
    OdriveSnapshot  left;
    OdriveSnapshot  right;
};

class Telemetry {
public:
    static const uint32_t COLLECT_PERIOD_MS = 150;
    static const uint32_t PRINT_PERIOD_MS   = 100;
    static const uint8_t  LOG_BUFFER_SIZE   = 16;

    Telemetry(Encoder &encoder, MotionController &motion, ODriveUART &left, ODriveUART &right);

    void begin();
    void update(); // вызывается на каждом проходе loop()

    void log(LogLevel level, const char* module, const char* msg);

private:
    Encoder &_encoder;
    MotionController &_motion;
    ODriveUART &_left;
    ODriveUART &_right;

    TelemetrySample _sample;
    uint32_t _lastCollectMs;
    uint32_t _lastPrintMs;

    LogLevel _minLevel;

    struct LogEntry {
        LogLevel level;
        char module[16];
        char msg[80];
    };
    LogEntry _logBuffer[LOG_BUFFER_SIZE];
    uint8_t _logHead;  // индекс следующей записи (по кругу)
    uint8_t _logCount;

    void collect();
    void printScheduled();
    void printImmediate(LogLevel level, const char* module, const char* msg);
    void printOdrive(const char* label, const OdriveSnapshot &s);
    const char* levelToStr(LogLevel level);
};

#endif