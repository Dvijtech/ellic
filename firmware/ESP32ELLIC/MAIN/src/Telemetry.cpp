#include "Telemetry.h"

Telemetry::Telemetry(Encoder &encoder, MotionController &motion, ODriveUART &left, ODriveUART &right)
: _encoder(encoder), _motion(motion), _left(left), _right(right),
  _lastCollectMs(0), _lastPrintMs(0), _minLevel(LogLevel::INFO),
  _logHead(0), _logCount(0)
{
    _sample.encoder = {0, 0, 0};
    _sample.motion.valDelta = 0;
    _sample.motion.leftWheelDelta = 0;
    _sample.motion.rightWheelDelta = 0;
    _sample.motion.mode = MotionSnapshot::Mode::CALM;
    _sample.motion.leftBrake = false;
    _sample.motion.rightBrake = false;
    _sample.motion.inTurnZone = false;
}

void Telemetry::begin() {
    _lastCollectMs = millis();
    _lastPrintMs = millis();
    collect();
}

void Telemetry::collect() {
    _sample.encoder = _encoder.getSnapshot();
    _sample.motion  = _motion.getSnapshot();
    _sample.left    = _left.getSnapshot();
    _sample.right   = _right.getSnapshot();
}

const char* Telemetry::levelToStr(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "?";
}

void Telemetry::printImmediate(LogLevel level, const char* module, const char* msg) {
    Serial.print('[');
    Serial.print(levelToStr(level));
    Serial.print("] ");
    Serial.print(module);
    Serial.print(": ");
    Serial.println(msg);
}

void Telemetry::log(LogLevel level, const char* module, const char* msg) {
    if ((uint8_t)level < (uint8_t)_minLevel) return;

    if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
        printImmediate(level, module, msg);
        return;
    }

    LogEntry &e = _logBuffer[_logHead];
    e.level = level;
    strncpy(e.module, module, sizeof(e.module) - 1);
    e.module[sizeof(e.module) - 1] = 0;
    strncpy(e.msg, msg, sizeof(e.msg) - 1);
    e.msg[sizeof(e.msg) - 1] = 0;

    _logHead = (_logHead + 1) % LOG_BUFFER_SIZE;
    if (_logCount < LOG_BUFFER_SIZE) _logCount++;
}

void Telemetry::printOdrive(const char* label, const OdriveSnapshot &s) {
    Serial.print("ODRIVE ");
    Serial.print(label);
    Serial.print(": online=");
    Serial.print(s.online ? "1" : "0");
    Serial.print(" state=");
    Serial.print(s.axisState);
    Serial.print(" axis_error=");
    Serial.print(s.axisError);
    Serial.print(" motor_error=");
    Serial.print(s.motorError);
    Serial.print(" controller_error=");
    Serial.print(s.controllerError);
    Serial.print(" tx=");
    Serial.print(s.txCount);
    Serial.print(" rx=");
    Serial.print(s.rxCount);
    Serial.print(" rx_fail=");
    Serial.println(s.rxFailCount);
}

void Telemetry::printScheduled() {
    Serial.print("ENC raw=");
    Serial.print(_sample.encoder.rawAngle, 2);
    Serial.print(" cont=");
    Serial.print(_sample.encoder.continuousAngle, 2);
    Serial.print(" delta=");
    Serial.println(_sample.encoder.lastDelta, 3);

    Serial.print("BRAKE left=");
    Serial.print(_sample.motion.leftBrake ? "1" : "0");
    Serial.print(" right=");
    Serial.print(_sample.motion.rightBrake ? "1" : "0");
    Serial.print(" both=");
    Serial.println((_sample.motion.leftBrake && _sample.motion.rightBrake) ? "1" : "0");

    Serial.print("TURN zone=");
    Serial.println(_sample.motion.inTurnZone ? "1" : "0");

    Serial.print("MOTION mode=");
    switch (_sample.motion.mode) {
        case MotionSnapshot::Mode::NORMAL: Serial.println("NORMAL"); break;
        case MotionSnapshot::Mode::CALM:   Serial.println("CALM");   break;
        case MotionSnapshot::Mode::TURN:   Serial.println("TURN");  break;
    }

    Serial.print("WHEEL delta left=");
    Serial.print(_sample.motion.leftWheelDelta, 4);
    Serial.print(" right=");
    Serial.println(_sample.motion.rightWheelDelta, 4);

    printOdrive("LEFT", _sample.left);
    printOdrive("RIGHT", _sample.right);

    uint8_t start = (_logHead + LOG_BUFFER_SIZE - _logCount) % LOG_BUFFER_SIZE;
    for (uint8_t i = 0; i < _logCount; i++) {
        LogEntry &e = _logBuffer[(start + i) % LOG_BUFFER_SIZE];
        Serial.print('[');
        Serial.print(levelToStr(e.level));
        Serial.print("] ");
        Serial.print(e.module);
        Serial.print(": ");
        Serial.println(e.msg);
    }
    _logCount = 0;
    _logHead = 0;
}

void Telemetry::update() {
    uint32_t now = millis();

    if (now - _lastCollectMs >= COLLECT_PERIOD_MS) {
        _lastCollectMs = now;
        collect();
    }

    if (now - _lastPrintMs >= PRINT_PERIOD_MS) {
        _lastPrintMs = now;
        printScheduled();
    }
}