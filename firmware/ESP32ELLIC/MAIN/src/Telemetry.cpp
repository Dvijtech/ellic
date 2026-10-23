#include "Telemetry.h"
#include "Encoder.h"
#include "MotionController.h"
#include "ODriveCAN.h"
#include "BleTelemetry.h"
#include "FireButton.h"
#include "BrakeInputs.h"
#include <esp_system.h>
#include <esp_attr.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

namespace {
// Память RTC без инициализации: переживает программный сброс, watchdog и
// brownout (но не полное отключение питания - тогда magic не совпадёт).
constexpr uint32_t RTC_MAGIC = 0xE111C0DE;
RTC_NOINIT_ATTR uint32_t rtcMagic;
RTC_NOINIT_ATTR uint32_t rtcBootCount;
RTC_NOINIT_ATTR uint32_t rtcLastUptimeS;

// Размер буфера пакета. Типичный пакет ~500 байт.
constexpr size_t PACKET_BUF = 900;
} // namespace

// Дописывает форматированный текст в buf начиная с pos. Если запись целиком не
// помещается - ничего не добавляет (чтобы не получилось оборванной строки).
static size_t appendf(char* buf, size_t cap, size_t pos, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));

static size_t appendf(char* buf, size_t cap, size_t pos, const char* fmt, ...) {
    if (pos >= cap) {
        return pos;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + pos, cap - pos, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap - pos) {
        buf[pos] = '\0';
        return pos;
    }
    return pos + (size_t)n;
}

Telemetry::Telemetry(Encoder* encoder, MotionController* motion, ODriveCAN* odriveCAN)
    : _encoder(encoder),
      _motion(motion),
      _odriveCAN(odriveCAN),
      _ble(nullptr),
      _fire(nullptr),
      _brakes(nullptr),
      _lastMs(0),
      _prevAs5600(0),
      _prevAsRecov(0),
      _prevBrakeL(0),
      _prevBrakeR(0),
      _loopGapMaxUs(0),
      _lastSendUs(0),
      _resetReason(0),
      _bootCount(0),
      _prevUptimeS(0) {
    memset(&_prevLeft, 0, sizeof(_prevLeft));
    memset(&_prevRight, 0, sizeof(_prevRight));
    memset(_prevCmdLeft, 0, sizeof(_prevCmdLeft));
    memset(_prevCmdRight, 0, sizeof(_prevCmdRight));
    memset(&_prevBus, 0, sizeof(_prevBus));
}

void Telemetry::setBleTelemetry(BleTelemetry* ble) {
    _ble = ble;
}

void Telemetry::setFireButton(FireButton* fire) {
    _fire = fire;
}

void Telemetry::setBrakeInputs(BrakeInputs* brakes) {
    _brakes = brakes;
}

void Telemetry::begin() {
    _lastMs = millis();

    _resetReason = (int)esp_reset_reason();
    // После полного отключения питания RTC-память содержит мусор.
    if (rtcMagic != RTC_MAGIC || _resetReason == (int)ESP_RST_POWERON) {
        rtcMagic = RTC_MAGIC;
        rtcBootCount = 0;
        rtcLastUptimeS = 0;
    }
    _prevUptimeS = rtcLastUptimeS;
    rtcBootCount++;
    _bootCount = rtcBootCount;
}

void Telemetry::noteLoopGap(uint32_t gapUs) {
    if (gapUs > _loopGapMaxUs) {
        _loopGapMaxUs = gapUs;
    }
}

// Единственная точка вывода: Serial (если включён) + BLE (если подключён).
void Telemetry::emit(const char* text) const {
#if TELEMETRY_TO_SERIAL
    Serial.print(text);
#endif
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

// M,ts,raw,cont,delta,valDelta,mode,flags,leftWheelDelta,rightWheelDelta,fire,fire_count
// flags: бит0 левый тормоз, бит1 правый тормоз, бит2 TURN_ZONE
// fire: 1 - кнопка стрельбы нажата; fire_count - число нажатий с запуска ESP.
// Одна функция для периодического пакета и для немедленной выдачи по кнопке,
// чтобы формат строки был гарантированно одинаковым.
static size_t appendMotion(char* buf, size_t cap, size_t pos, uint32_t ts,
                           const EncoderSnapshot &enc, const MotionSnapshot &mo,
                           const FireSnapshot &fs) {
    int flags = (mo.leftBrake ? 1 : 0) | (mo.rightBrake ? 2 : 0) | (mo.inTurnZone ? 4 : 0);
    return appendf(buf, cap, pos, "M,%lu,%.1f,%.1f,%.2f,%.2f,%d,%d,%.3f,%.3f,%d,%lu\n",
                   (unsigned long)ts, enc.rawAngle, enc.continuousAngle, enc.lastDelta,
                   mo.valDelta, modeToInt(mo.mode), flags,
                   mo.leftWheelDelta, mo.rightWheelDelta,
                   fs.pressed ? 1 : 0, (unsigned long)fs.pressCount);
}

// L/R,ts,online,state,traj,iq,vel,vbus,ibus,axisErr,motorErr,encErr,ctrlErr,tx,rx
// tx/rx - число кадров за период (не накопительно).
static size_t appendWheel(char* buf, size_t cap, size_t pos, char type, uint32_t ts,
                          const OdriveSnapshot &s, uint32_t txDelta, uint32_t rxDelta) {
    return appendf(buf, cap, pos,
                   "%c,%lu,%d,%d,%d,%.2f,%.3f,%.2f,%.2f,%lX,%llX,%lX,%lX,%lu,%lu\n",
                   type, (unsigned long)ts,
                   s.online ? 1 : 0, s.axisState, s.trajectoryDone ? 1 : 0,
                   s.Iq, s.velEstimate, s.busVoltage, s.busCurrent,
                   (unsigned long)s.axisError, (unsigned long long)s.motorError,
                   (unsigned long)s.encoderError, (unsigned long)s.controllerError,
                   (unsigned long)txDelta, (unsigned long)rxDelta);
}

// C: счётчики кадров по типам (за период) - сначала 7 для L, потом 7 для R.
static size_t appendFrameCounts(char* buf, size_t cap, size_t pos, uint32_t ts,
                                const uint32_t* nowL, const uint32_t* prevL,
                                const uint32_t* nowR, const uint32_t* prevR) {
    pos = appendf(buf, cap, pos, "C,%lu", (unsigned long)ts);
    for (uint8_t i = 0; i < ODRIVE_CMD_SLOTS; i++) {
        pos = appendf(buf, cap, pos, ",%lu", (unsigned long)(nowL[i] - prevL[i]));
    }
    for (uint8_t i = 0; i < ODRIVE_CMD_SLOTS; i++) {
        pos = appendf(buf, cap, pos, ",%lu", (unsigned long)(nowR[i] - prevR[i]));
    }
    return appendf(buf, cap, pos, "\n");
}

void Telemetry::sendPacket(uint32_t ts) {
    EncoderSnapshot enc = _encoder->getSnapshot();
    MotionSnapshot mo = _motion->getSnapshot();
    OdriveSnapshot l = _odriveCAN->getLeftSnapshot();
    OdriveSnapshot r = _odriveCAN->getRightSnapshot();
    CanBusSnapshot bus = _odriveCAN->getBusSnapshot();

    WheelCounters cl = countersOf(l);
    WheelCounters cr = countersOf(r);

    char pkt[PACKET_BUF];
    size_t pos = 0;
    pkt[0] = '\0';

    FireSnapshot fs = { false, 0 };
    if (_fire != nullptr) {
        fs = _fire->getSnapshot();
    }
    BrakeSnapshot bs = { false, false, 0, 0, 0 };
    if (_brakes != nullptr) {
        bs = _brakes->getSnapshot();
    }
    pos = appendMotion(pkt, sizeof(pkt), pos, ts, enc, mo, fs);

    pos = appendWheel(pkt, sizeof(pkt), pos, 'L', ts, l, cl.tx - _prevLeft.tx, cl.rx - _prevLeft.rx);
    pos = appendWheel(pkt, sizeof(pkt), pos, 'R', ts, r, cr.tx - _prevRight.tx, cr.rx - _prevRight.rx);

    // E,ts,as5600,lTxFail,rTxFail,lStale,rStale,lOffline,rOffline,
    //   lAxisErr,rAxisErr,lMotErr,rMotErr,lEncErr,rEncErr,lCtrlErr,rCtrlErr,
    //   lBrakeGlitch,rBrakeGlitch
    // lBrakeGlitch/rBrakeGlitch - импульсы на входе тормоза, отброшенные фильтром
    // (короче BRAKE_DEBOUNCE_MS), за период. Значения - число событий с прошлой выдачи (разность uint32 корректна
    // и при переполнении счётчика).
    pos = appendf(pkt, sizeof(pkt), pos,
                  "E,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
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
                  (unsigned long)(cr.ctrlErr - _prevRight.ctrlErr),
                  (unsigned long)(bs.leftGlitches - _prevBrakeL),
                  (unsigned long)(bs.rightGlitches - _prevBrakeR));

    // D,ts,uptimeS,reset,boots,prevUptimeS,loopGapMaxUs,sendUs,asReadMaxUs,
    //   twaiState,tec,rec,rxMissed,arbLost,busError,txFailedHw,busOffRecov,ageL,ageR,
    //   canStalls,asRecov,brakeGlitchMaxMs
    // uptimeS/reset/boots/prevUptimeS/twaiState/tec/rec - текущие значения;
    // loopGapMaxUs/sendUs/asReadMaxUs/ageL/ageR - максимумы за период;
    // rxMissed/arbLost/busError/txFailedHw/busOffRecov - число событий за период;
    // canStalls - сколько раз позиции колёс сбрасывались после паузы loop() или
    // потери кадров; asRecov - сколько раз восстанавливалась шина I2C (за период).
    pos = appendf(pkt, sizeof(pkt), pos,
                  "D,%lu,%lu,%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                  (unsigned long)ts,
                  (unsigned long)(ts / 1000UL),
                  _resetReason,
                  (unsigned long)_bootCount,
                  (unsigned long)_prevUptimeS,
                  (unsigned long)_loopGapMaxUs,
                  (unsigned long)_lastSendUs,
                  (unsigned long)enc.readMaxUs,
                  (unsigned long)bus.state,
                  (unsigned long)bus.txErrCounter,
                  (unsigned long)bus.rxErrCounter,
                  (unsigned long)(bus.rxMissed - _prevBus.rxMissed),
                  (unsigned long)(bus.arbLost - _prevBus.arbLost),
                  (unsigned long)(bus.busError - _prevBus.busError),
                  (unsigned long)(bus.txFailedHw - _prevBus.txFailedHw),
                  (unsigned long)(bus.busOffRecoveries - _prevBus.busOffRecoveries),
                  (unsigned long)l.posAgeMaxMs,
                  (unsigned long)r.posAgeMaxMs,
                  (unsigned long)(bus.posInvalidations - _prevBus.posInvalidations),
                  (unsigned long)(enc.busRecoveries - _prevAsRecov),
                  (unsigned long)bs.glitchMaxMs);

    // C,ts,L[hb,motErr,encErr,est,iq,busVI,ctrlErr],R[то же]
    // Сколько кадров каждого типа принято за период (норма при rate_ms=100 и
    // периоде 200 мс - по 2 кадра каждого типа).
    pos = appendFrameCounts(pkt, sizeof(pkt), pos, ts,
                            l.cmdCount, _prevCmdLeft, r.cmdCount, _prevCmdRight);

    emit(pkt);

    // Запоминаем значения для расчёта событий за следующий период.
    _prevAs5600 = enc.readErrors;
    _prevAsRecov = enc.busRecoveries;
    _prevBrakeL = bs.leftGlitches;
    _prevBrakeR = bs.rightGlitches;
    _prevLeft = cl;
    _prevRight = cr;
    _prevBus = bus;
    for (uint8_t i = 0; i < ODRIVE_CMD_SLOTS; i++) {
        _prevCmdLeft[i] = l.cmdCount[i];
        _prevCmdRight[i] = r.cmdCount[i];
    }

    // Максимумы - за период: начинаем заново.
    _loopGapMaxUs = 0;
    _encoder->resetPeriodStats();
    _odriveCAN->resetPeriodStats();
    if (_brakes != nullptr) {
        _brakes->resetPeriodStats();
    }
}

// Немедленная выдача при изменении состояния кнопки: одна строка M (~80 байт)
// вместо полного пакета - минимальная задержка и минимальная нагрузка на
// Serial/BLE. Периодический график выдачи от неё не сдвигается.
void Telemetry::sendFireEvent() {
    char line[160];
    FireSnapshot fs = { false, 0 };
    if (_fire != nullptr) {
        fs = _fire->getSnapshot();
    }
    size_t n = appendMotion(line, sizeof(line), 0, millis(),
                            _encoder->getSnapshot(), _motion->getSnapshot(), fs);
    if (n > 0) {
        emit(line);
    }
}

void Telemetry::update() {
    uint32_t now = millis();
    if (now - _lastMs >= TELEMETRY_PERIOD_MS) {
        _lastMs = now;
        uint32_t t0 = micros();
        sendPacket(now);
        _lastSendUs = micros() - t0;
        rtcLastUptimeS = now / 1000UL;
    }
}
