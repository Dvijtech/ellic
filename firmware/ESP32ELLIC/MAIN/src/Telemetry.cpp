#include "Telemetry.h"
#include <cstring>

Telemetry::Telemetry(Encoder& encoder, MotionController& motion, ODriveCAN& odrive)
    : encoder_(encoder),
      motion_(motion),
      odrive_(odrive),
      lastCollectMs_(0),
      lastPrintMs_(0),
      hasSample_(false),
      logHead_(0),
      logCount_(0),
      minLevel_(LogLevel::INFO) {}

void Telemetry::begin() {
    lastCollectMs_ = millis();
    lastPrintMs_ = millis();
    collect();
}

void Telemetry::update() {
    const uint32_t now = millis();

    if (static_cast<uint32_t>(now - lastCollectMs_) >= periodMs) {
        lastCollectMs_ = now;
        collect();
    }

    if (static_cast<uint32_t>(now - lastPrintMs_) >= printPeriodMs) {
        lastPrintMs_ = now;
        printScheduled();
    }
}

void Telemetry::collect() {
    sample_.encoder = encoder_.getSnapshot();
    sample_.motion = motion_.getSnapshot();
    sample_.left = odrive_.getSnapshotLeft();
    sample_.right = odrive_.getSnapshotRight();
    hasSample_ = true;
}

void Telemetry::printScheduled() {
    if (hasSample_) {
        Serial.println("--------------------------------");
        Serial.printf("ENC raw=%.2f\n", sample_.encoder.rawAngle);
        Serial.printf("ENC cont=%.2f\n", sample_.encoder.continuousAngle);
        Serial.printf("ENC delta=%.4f\n", sample_.encoder.lastDelta);

        const bool both = sample_.motion.leftBrake && sample_.motion.rightBrake;
        Serial.printf("BRAKE left=%d right=%d both=%d\n",
                      sample_.motion.leftBrake,
                      sample_.motion.rightBrake,
                      both);
        Serial.printf("TURN zone=%d\n", sample_.motion.inTurnZone);
        Serial.printf("MOTION mode=%s\n", modeName(sample_.motion.mode));
        Serial.printf("WHEEL delta left=%.4f right=%.4f\n",
                      sample_.motion.leftWheelDelta,
                      sample_.motion.rightWheelDelta);

        printOdrive("LEFT", sample_.left);
        printOdrive("RIGHT", sample_.right);
    }

    while (logCount_ > 0) {
        const size_t index = (logHead_ + LOG_BUFFER_SIZE - logCount_) % LOG_BUFFER_SIZE;
        printLogEntry(logBuffer_[index]);
        --logCount_;
    }
}

void Telemetry::log(LogLevel level, const char* module, const char* msg) {
    if (!levelAllowed(level, minLevel_)) {
        return;
    }

    if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
        Serial.printf("[%s] %s: %s\n", levelName(level), module ? module : "?", msg ? msg : "");
        return;
    }

    LogEntry& entry = logBuffer_[logHead_];
    entry.level = level;
    snprintf(entry.module, sizeof(entry.module), "%s", module ? module : "?");
    snprintf(entry.message, sizeof(entry.message), "%s", msg ? msg : "");

    logHead_ = (logHead_ + 1) % LOG_BUFFER_SIZE;
    if (logCount_ < LOG_BUFFER_SIZE) {
        ++logCount_;
    }
}

const TelemetrySample& Telemetry::getSample() const {
    return sample_;
}

void Telemetry::printLogEntry(const LogEntry& entry) {
    Serial.printf("[%s] %s: %s\n", levelName(entry.level), entry.module, entry.message);
}

void Telemetry::printOdrive(const char* name, const OdriveSnapshot& snapshot) {
    Serial.printf("ODRIVE %s\n", name);
    Serial.printf(" online=%d\n", snapshot.online);
    Serial.printf(" state=%d\n", snapshot.axisState);
    Serial.printf(" axis_error=%lu\n", static_cast<unsigned long>(snapshot.axisError));
    Serial.printf(" motor_error=%lu\n", static_cast<unsigned long>(snapshot.motorError));
    Serial.printf(" encoder_error=%lu\n", static_cast<unsigned long>(snapshot.encoderError));
    Serial.printf(" controller_error=%lu\n", static_cast<unsigned long>(snapshot.controllerError));
    Serial.printf(" trajectory_done=%d\n", snapshot.trajectoryDone);
    Serial.printf(" position=%.5f valid=%d age=%lu ms\n",
                  snapshot.currentPosition,
                  snapshot.positionValid,
                  static_cast<unsigned long>(millis() - snapshot.positionTimestampMs));
    Serial.printf(" velocity=%.5f\n", snapshot.velEstimate);
    Serial.printf(" iq=%.4f\n", snapshot.iq);
    Serial.printf(" bus_voltage=%.3f\n", snapshot.busVoltage);
    Serial.printf(" bus_current=%.3f\n", snapshot.busCurrent);
    Serial.printf(" tx=%lu rx=%lu rx_fail=%lu\n",
                  static_cast<unsigned long>(snapshot.txCount),
                  static_cast<unsigned long>(snapshot.rxCount),
                  static_cast<unsigned long>(snapshot.rxFailCount));
}

bool Telemetry::levelAllowed(LogLevel level, LogLevel minLevel) {
    return static_cast<int>(level) >= static_cast<int>(minLevel);
}

const char* Telemetry::levelName(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "?";
    }
}

const char* Telemetry::modeName(MotionSnapshot::Mode mode) {
    switch (mode) {
        case MotionSnapshot::Mode::NORMAL: return "NORMAL";
        case MotionSnapshot::Mode::CALM: return "CALM";
        case MotionSnapshot::Mode::TURN: return "TURN";
        default: return "?";
    }
}
