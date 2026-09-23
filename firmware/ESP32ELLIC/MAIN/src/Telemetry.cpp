#include "Telemetry.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"
#include "BleTelemetry.h"
#include <string.h>
#include <stdio.h>

Telemetry::Telemetry(Encoder* encoder, MotionController* motion, ODriveCAN* odriveCAN)
    : _encoder(encoder),
      _motion(motion),
      _odriveCAN(odriveCAN),
      _ble(nullptr),
      _lastCollectMs(0),
      _lastPrintMs(0),
      _minLevel(LogLevel::INFO),
      _logWriteIndex(0),
      _logCount(0) {
    memset(&_sample, 0, sizeof(_sample));
}

void Telemetry::setBleTelemetry(BleTelemetry* ble) {
    _ble = ble;
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
    _sample.left    = _odriveCAN->getLeftSnapshot();
    _sample.right   = _odriveCAN->getRightSnapshot();
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

// Единственная точка, где текст телеметрии реально уходит наружу.
// Serial.print() тут же, плюс дублирование в BLE, если он подключён -
// таким образом состав данных не может разойтись между каналами: он
// формируется один раз (snprintf) и просто передаётся в оба места.
void Telemetry::emit(const char* text) const {
    Serial.print(text);
    if (_ble != nullptr) {
        _ble->send(text);
    }
}

static void formatOdrive(char* buf, size_t bufSize, const char* name, const OdriveSnapshot &s) {
    snprintf(buf, bufSize,
             "ODRIVE %s: online=%d state=%d axis_err=%lu motor_err=%llu enc_err=%lu ctrl_err=%lu traj_done=%d "
             "Iq=%.3f vel=%.3f Vbus=%.2f Ibus=%.3f tx=%lu rx=%lu rx_fail=%lu diag_ts=%lu\n",
             name, s.online ? 1 : 0, s.axisState,
             (unsigned long)s.axisError, (unsigned long long)s.motorError,
             (unsigned long)s.encoderError, (unsigned long)s.controllerError,
             s.trajectoryDone ? 1 : 0,
             s.Iq, s.velEstimate, s.busVoltage, s.busCurrent,
             (unsigned long)s.txCount, (unsigned long)s.rxCount,
             (unsigned long)s.rxFailCount, (unsigned long)s.diagnosticsTimestampMs);
}

void Telemetry::printSample() const {
    const EncoderSnapshot &enc = _sample.encoder;
    const MotionSnapshot &mo = _sample.motion;
    char line[220];

    snprintf(line, sizeof(line), "ENC raw=%.2f cont=%.2f delta=%.3f\n",
             enc.rawAngle, enc.continuousAngle, enc.lastDelta);
    emit(line);

    snprintf(line, sizeof(line), "BRAKE left=%d right=%d both=%d TURN zone=%d\n",
             mo.leftBrake ? 1 : 0, mo.rightBrake ? 1 : 0,
             (mo.leftBrake && mo.rightBrake) ? 1 : 0, mo.inTurnZone ? 1 : 0);
    emit(line);

    snprintf(line, sizeof(line), "MOTION mode=%s valDelta=%.4f\n", modeToStr(mo.mode), mo.valDelta);
    emit(line);

    snprintf(line, sizeof(line), "WHEEL delta left=%.4f right=%.4f\n",
             mo.leftWheelDelta, mo.rightWheelDelta);
    emit(line);

    formatOdrive(line, sizeof(line), "LEFT", _sample.left);
    emit(line);

    formatOdrive(line, sizeof(line), "RIGHT", _sample.right);
    emit(line);
}

void Telemetry::printLogBuffer() {
    int start = (_logWriteIndex - _logCount + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    char line[128];
    for (int i = 0; i < _logCount; i++) {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        const LogEntry &e = _logBuffer[idx];
        snprintf(line, sizeof(line), "[%s] %s: %s\n", levelToStr(e.level), e.module, e.msg);
        emit(line);
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
        char line[128];
        snprintf(line, sizeof(line), "[%s] %s: %s\n", levelToStr(level), module, msg);
        emit(line);
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
}
