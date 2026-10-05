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
      _lastMs(0),
      _prevAs5600(0) {
    memset(&_prevLeft, 0, sizeof(_prevLeft));
    memset(&_prevRight, 0, sizeof(_prevRight));
}

void Telemetry::setBleTelemetry(BleTelemetry* ble) {
    _ble = ble;
}

void Telemetry::begin() {
    _lastMs = millis();
}

// Единственная точка вывода: Serial + (если подключён) BLE, один и тот же текст.
void Telemetry::emit(const char* text) const {
    Serial.print(text);
    if (_ble != nullptr) {
        _ble->send(text);
    }
}

Telemetry::WheelCounters Telemetry::countersOf(const OdriveSnapshot &s) {
    WheelCounters c;
    c.tx = s.txCount;
    c.rx = s.rxCount;
    c.txFail = s.txFailCount;
    c.stale = s.rxFailCount;
    c.offline = s.offlineCount;
    c.axisErr = s.axisErrCount;
    c.motErr = s.motorErrCount;
    c.encErr = s.encoderErrCount;
    c.ctrlErr = s.controllerErrCount;
    return c;
}

static int modeToInt(MotionMode m) {
    switch (m) {
        case MotionMode::NORMAL: return 0;
        case MotionMode::CALM:   return 1;
        case MotionMode::TURN:   return 2;
    }
    return 0;
}

// L/R,ts,online,state,traj,iq,vel,vbus,ibus,axisErr,motorErr,encErr,ctrlErr,tx,rx
// tx/rx - число кадров за период (не накопительно).
static void formatWheel(char* buf, size_t bufSize, char type, uint32_t ts,
                        const OdriveSnapshot &s, uint32_t txDelta, uint32_t rxDelta) {
    snprintf(buf, bufSize,
             "%c,%lu,%d,%d,%d,%.2f,%.3f,%.2f,%.2f,%lX,%llX,%lX,%lX,%lu,%lu\n",
             type, (unsigned long)ts,
             s.online ? 1 : 0, s.axisState, s.trajectoryDone ? 1 : 0,
             s.Iq, s.velEstimate, s.busVoltage, s.busCurrent,
             (unsigned long)s.axisError, (unsigned long long)s.motorError,
             (unsigned long)s.encoderError, (unsigned long)s.controllerError,
             (unsigned long)txDelta, (unsigned long)rxDelta);
}

void Telemetry::sendPacket(uint32_t ts) {
    EncoderSnapshot enc = _encoder->getSnapshot();
    MotionSnapshot mo = _motion->getSnapshot();
    OdriveSnapshot l = _odriveCAN->getLeftSnapshot();
    OdriveSnapshot r = _odriveCAN->getRightSnapshot();

    WheelCounters cl = countersOf(l);
    WheelCounters cr = countersOf(r);
    char line[200];

    // M,ts,raw,cont,delta,valDelta,mode,flags,leftWheelDelta,rightWheelDelta
    // flags: бит0 левый тормоз, бит1 правый тормоз, бит2 TURN_ZONE
    int flags = (mo.leftBrake ? 1 : 0) | (mo.rightBrake ? 2 : 0) | (mo.inTurnZone ? 4 : 0);
    snprintf(line, sizeof(line), "M,%lu,%.1f,%.1f,%.2f,%.2f,%d,%d,%.3f,%.3f\n",
             (unsigned long)ts, enc.rawAngle, enc.continuousAngle, enc.lastDelta,
             mo.valDelta, modeToInt(mo.mode), flags,
             mo.leftWheelDelta, mo.rightWheelDelta);
    emit(line);

    formatWheel(line, sizeof(line), 'L', ts, l, cl.tx - _prevLeft.tx, cl.rx - _prevLeft.rx);
    emit(line);

    formatWheel(line, sizeof(line), 'R', ts, r, cr.tx - _prevRight.tx, cr.rx - _prevRight.rx);
    emit(line);

    // E,ts,as5600,lTxFail,rTxFail,lStale,rStale,lOffline,rOffline,
    //   lAxisErr,rAxisErr,lMotErr,rMotErr,lEncErr,rEncErr,lCtrlErr,rCtrlErr
    // Значения - число событий с прошлой выдачи (разность uint32 корректна
    // и при переполнении счётчика).
    snprintf(line, sizeof(line),
             "E,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
             (unsigned long)ts,
             (unsigned long)(enc.readErrors - _prevAs5600),
             (unsigned long)(cl.txFail - _prevLeft.txFail),
             (unsigned long)(cr.txFail - _prevRight.txFail),
             (unsigned long)(cl.stale - _prevLeft.stale),
             (unsigned long)(cr.stale - _prevRight.stale),
             (unsigned long)(cl.offline - _prevLeft.offline),
             (unsigned long)(cr.offline - _prevRight.offline),
             (unsigned long)(cl.axisErr - _prevLeft.axisErr),
             (unsigned long)(cr.axisErr - _prevRight.axisErr),
             (unsigned long)(cl.motErr - _prevLeft.motErr),
             (unsigned long)(cr.motErr - _prevRight.motErr),
             (unsigned long)(cl.encErr - _prevLeft.encErr),
             (unsigned long)(cr.encErr - _prevRight.encErr),
             (unsigned long)(cl.ctrlErr - _prevLeft.ctrlErr),
             (unsigned long)(cr.ctrlErr - _prevRight.ctrlErr));
    emit(line);

    _prevAs5600 = enc.readErrors;
    _prevLeft = cl;
    _prevRight = cr;
}

void Telemetry::update() {
    uint32_t now = millis();
    if (now - _lastMs >= TELEMETRY_PERIOD_MS) {
        _lastMs = now;
        sendPacket(now);
    }
}