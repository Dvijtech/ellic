#pragma once

#include <Arduino.h>
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"

// ERROR/CRITICAL are printed immediately; INFO/WARNING are buffered.
enum class LogLevel { INFO, WARNING, ERROR, CRITICAL };

struct TelemetrySample {
    EncoderSnapshot encoder;
    MotionSnapshot motion;
    OdriveSnapshot left;
    OdriveSnapshot right;
};

class Telemetry {
public:
    static constexpr uint32_t periodMs = 150;
    static constexpr uint32_t printPeriodMs = 100;
    static constexpr size_t LOG_BUFFER_SIZE = 16;
    static constexpr size_t LOG_TEXT_SIZE = 96;

    Telemetry(Encoder& encoder, MotionController& motion, ODriveCAN& odrive);

    void begin();
    void update();
    void log(LogLevel level, const char* module, const char* msg);

    const TelemetrySample& getSample() const;

private:
    struct LogEntry {
        LogLevel level;
        char module[24];
        char message[LOG_TEXT_SIZE];
    };

    Encoder& encoder_;
    MotionController& motion_;
    ODriveCAN& odrive_;

    TelemetrySample sample_{};
    uint32_t lastCollectMs_;
    uint32_t lastPrintMs_;
    bool hasSample_;

    LogEntry logBuffer_[LOG_BUFFER_SIZE];
    size_t logHead_;
    size_t logCount_;
    LogLevel minLevel_;

    void collect();
    void printScheduled();
    void printLogEntry(const LogEntry& entry);
    void printOdrive(const char* name, const OdriveSnapshot& snapshot);
    static bool levelAllowed(LogLevel level, LogLevel minLevel);
    static const char* levelName(LogLevel level);
    static const char* modeName(MotionSnapshot::Mode mode);
};
