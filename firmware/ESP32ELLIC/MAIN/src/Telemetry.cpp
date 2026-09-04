#include "Telemetry.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveUART.h"
#include <string.h>
#include <stdio.h>

Telemetry::Telemetry(Encoder* encoder, MotionController* motion,
                      ODriveUART* left, ODriveUART* right)
    : _encoder(encoder),
      _motion(motion),
      _left(left),
      _right(right),
      _lastCollectMs(0),
      _lastPrintMs(0),
      _minLevel(LogLevel::INFO),
      _logWriteIndex(0),
      _logCount(0) {
    memset(&_sample, 0, sizeof(_sample));
}

void Telemetry::begin(LogLevel minLevel) {
    _minLevel = minLevel;
    _lastCollectMs = millis();
    _lastPrintMs = millis();
    _logCount = 0;
}

void Telemetry::collect() {
    _sample.encoder = _encoder->getSnapshot();
    _sample.motion  = _motion->getSnapshot();
    _sample.left    = _left->getSnapshot();
    _sample.right   = _right->getSnapshot();
}

static const char* modeToStr(MotionMode m) {
    switch (m) {
        case MotionMode::NORMAL: return "NORMAL";
        case MotionMode::CALM:   return "CALM";
        case MotionMode::TURN:   return "TURN";
    }
    return "?";
}

static const char* levelToStr(LogLevel l) {
    switch (l) {
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "?";
}

static void printOdrive(const char* name, const OdriveSnapshot &s) {
    Serial.printf("ODRIVE %s: online=%d state=%d axis_err=%d motor_err=%d ctrl_err=%d "
                  "Iq=%.3f Vq=%.3f vel=%.3f tx=%lu rx=%lu rx_fail=%lu diag_ts=%lu\n",
                  name, s.online ? 1 : 0, s.axisState, s.axisError, s.motorError,
                  s.controllerError, s.Iq, s.Vq, s.velEstimate,
                  (unsigned long)s.txCount, (unsigned long)s.rxCount,
                  (unsigned long)s.rxFailCount, (unsigned long)s.diagnosticsTimestampMs);
}

void Telemetry::printSample() const {
    const EncoderSnapshot &enc = _sample.encoder;
    const MotionSnapshot &mo = _sample.motion;

    Serial.printf("ENC raw=%.2f cont=%.2f delta=%.3f\n",
                  enc.rawAngle, enc.continuousAngle, enc.lastDelta);
    Serial.printf("BRAKE left=%d right=%d both=%d TURN zone=%d\n",
                  mo.leftBrake ? 1 : 0, mo.rightBrake ? 1 : 0,
                  (mo.leftBrake && mo.rightBrake) ? 1 : 0, mo.inTurnZone ? 1 : 0);
    Serial.printf("MOTION mode=%s valDelta=%.4f\n", modeToStr(mo.mode), mo.valDelta);
    Serial.printf("WHEEL delta left=%.4f right=%.4f\n",
                  mo.leftWheelDelta, mo.rightWheelDelta);

    printOdrive("LEFT", _sample.left);
    printOdrive("RIGHT", _sample.right);
}

void Telemetry::printLogBuffer() {
    // Печатаем накопленные записи в хронологическом порядке.
    int start = (_logWriteIndex - _logCount + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    for (int i = 0; i < _logCount; i++) {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        const LogEntry &e = _logBuffer[idx];
        Serial.printf("[%s] %s: %s\n", levelToStr(e.level), e.module, e.msg);
    }
    _logCount = 0;
}

void Telemetry::printScheduled() {
    printSample();
    printLogBuffer();
}

void Telemetry::update() {
    uint32_t now = millis();

    if (now - _lastCollectMs >= TELEMETRY_PERIOD_MS) {
        _lastCollectMs = now;
        collect();
    }

    if (now - _lastPrintMs >= TELEMETRY_PRINT_PERIOD_MS) {
        _lastPrintMs = now;
        printScheduled();
    }
}

void Telemetry::log(LogLevel level, const char* module, const char* msg) {
    if (level < _minLevel) {
        return;
    }

    if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
        // Раздел 12.2: печатаются немедленно, синхронно, минуя расписание.
        Serial.printf("[%s] %s: %s\n", levelToStr(level), module, msg);
        return;
    }

    // INFO / WARNING - в кольцевой буфер до ближайшего printScheduled().
    int idx = _logWriteIndex;
    _logBuffer[idx].level = level;
    strncpy(_logBuffer[idx].module, module, sizeof(_logBuffer[idx].module) - 1);
    _logBuffer[idx].module[sizeof(_logBuffer[idx].module) - 1] = '\0';
    strncpy(_logBuffer[idx].msg, msg, sizeof(_logBuffer[idx].msg) - 1);
    _logBuffer[idx].msg[sizeof(_logBuffer[idx].msg) - 1] = '\0';

    _logWriteIndex = (_logWriteIndex + 1) % LOG_BUFFER_SIZE;
    if (_logCount < LOG_BUFFER_SIZE) {
        _logCount++;
    }
    // если между двумя printScheduled() записей больше, чем LOG_BUFFER_SIZE,
    // самые старые из них молча вытесняются более новыми (фикс. размер буфера).
}
