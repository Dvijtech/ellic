#include "Encoder.h"
#include "Telemetry.h"
#include <Wire.h>

namespace {
const BaseType_t  ENCODER_TASK_CORE = 0;
const uint32_t    ENCODER_TASK_STACK = 4096;
const UBaseType_t ENCODER_TASK_PRIORITY = 2;   // выше BLE-задачи (1): опрос датчика по расписанию
const uint32_t    ENCODER_SLOW_RETRY_MS = 100; // период повторов при длительной потере датчика
} // namespace

Encoder::Encoder()
    : _telemetry(nullptr),
      _task(nullptr),
      _initialized(false),
      _previousRawAngle(0.0f),
      _continuousAngle(0.0f),
      _lastDelta(0.0f),
      _hasOk(false),
      _lastOkMs(0),
      _failStreak(0),
      _pubRaw(0.0f),
      _pubCont(0.0f),
      _pubDelta(0.0f),
      _pubErrors(0),
      _pubReadMaxUs(0),
      _pubRecoveries(0) {}

void Encoder::begin(Telemetry* telemetry) {
    _telemetry = telemetry;
    if (_task != nullptr) {
        return; // задача уже запущена
    }
    // Wire.begin() и setClock() вызваны в setup() до этого места.
    Wire.setTimeOut(AS5600_I2C_TIMEOUT_MS);
    xTaskCreatePinnedToCore(
        &Encoder::taskEntry,
        "As5600",
        ENCODER_TASK_STACK,
        this,
        ENCODER_TASK_PRIORITY,
        &_task,
        ENCODER_TASK_CORE);
}

void Encoder::taskEntry(void* param) {
    static_cast<Encoder*>(param)->taskLoop();
}

// Чтение 12-битного RAW ANGLE регистра AS5600 (0x0C/0x0D) -> градусы 0..360.
// Возвращает false, если обмен не удался или пришло менее 2 байт (раздел 6.3).
bool Encoder::readRawAngleDeg(float &outDeg) {
    Wire.beginTransmission(AS5600_I2C_ADDR);
    Wire.write(AS5600_RAW_ANGLE_REG);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t received = Wire.requestFrom((int)AS5600_I2C_ADDR, (int)2);
    if (received < 2) {
        return false;
    }

    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    uint16_t raw12 = ((uint16_t)(hi & 0x0F) << 8) | lo;

    outDeg = (raw12 * 360.0f) / 4096.0f;
    return true;
}

// Освобождение шины I2C, если датчик удерживает SDA низким (прерванная
// транзакция): до 9 тактов на SCL, пока SDA не освободится, затем STOP.
void Encoder::busClear() {
    pinMode(AS5600_SDA_PIN, INPUT_PULLUP);
    pinMode(AS5600_SCL_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(AS5600_SCL_PIN, HIGH);
    delayMicroseconds(10);

    for (int i = 0; i < 9 && digitalRead(AS5600_SDA_PIN) == LOW; i++) {
        digitalWrite(AS5600_SCL_PIN, LOW);
        delayMicroseconds(10);
        digitalWrite(AS5600_SCL_PIN, HIGH);
        delayMicroseconds(10);
    }

    // STOP: SDA 0 -> 1 при SCL = 1
    pinMode(AS5600_SDA_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(AS5600_SDA_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(AS5600_SCL_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(AS5600_SDA_PIN, HIGH);
    delayMicroseconds(10);

    pinMode(AS5600_SDA_PIN, INPUT_PULLUP);
    pinMode(AS5600_SCL_PIN, INPUT_PULLUP);
}

void Encoder::recoverBus() {
    Wire.end();
    busClear();
    Wire.begin(AS5600_SDA_PIN, AS5600_SCL_PIN);
    Wire.setClock(AS5600_I2C_CLOCK_HZ);
    Wire.setTimeOut(AS5600_I2C_TIMEOUT_MS);
}

void Encoder::taskLoop() {
    for (;;) {
        float raw = 0.0f;
        uint32_t t0 = micros();
        bool ok = readRawAngleDeg(raw);
        uint32_t dur = micros() - t0;
        uint32_t nowMs = millis();

        if (ok) {
            _failStreak = 0;
            if (!_initialized) {
                // Первый успешный запуск: фиксируем точку отсчёта, дельту не считаем.
                _previousRawAngle = raw;
                _continuousAngle = 0.0f;
                _lastDelta = 0.0f;
                _initialized = true;
            } else if (_hasOk && (nowMs - _lastOkMs) > ENCODER_RESYNC_MS) {
                // Датчик не отвечал дольше ENCODER_RESYNC_MS: что вал делал за
                // это время, неизвестно (переход через 180° неоднозначен), поэтому
                // только перепривязываем точку отсчёта, дельту не считаем.
                _previousRawAngle = raw;
                _lastDelta = 0.0f;
            } else {
                float delta = raw - _previousRawAngle;
                if (delta > 180.0f) {
                    delta -= 360.0f;
                } else if (delta < -180.0f) {
                    delta += 360.0f;
                }
                _continuousAngle += delta;
                _lastDelta = delta;
                _previousRawAngle = raw;
            }
            _hasOk = true;
            _lastOkMs = nowMs;
        } else {
            _failStreak++;
        }

        portENTER_CRITICAL(&_mux);
        if (dur > _pubReadMaxUs) {
            _pubReadMaxUs = dur;
        }
        if (ok) {
            _pubRaw = _previousRawAngle;
            _pubCont = _continuousAngle;
            _pubDelta = _lastDelta;
        } else {
            _pubErrors++;
        }
        portEXIT_CRITICAL(&_mux);

        if (!ok) {
            // Раздел 6.3: continuousAngle и previousRawAngle не обновлялись;
            // освобождаем шину и переинициализируем Wire.
            recoverBus();
            portENTER_CRITICAL(&_mux);
            _pubRecoveries++;
            portEXIT_CRITICAL(&_mux);
        }

        // При длительной потере датчика (отключён провод) повторяем реже.
        uint32_t waitMs = (_failStreak > 3) ? ENCODER_SLOW_RETRY_MS : ENCODER_PERIOD_MS;
        vTaskDelay(pdMS_TO_TICKS(waitMs));
    }
}

EncoderSnapshot Encoder::getSnapshot() const {
    EncoderSnapshot s;
    portENTER_CRITICAL(&_mux);
    s.rawAngle = _pubRaw;
    s.continuousAngle = _pubCont;
    s.lastDelta = _pubDelta;
    s.readErrors = _pubErrors;
    s.readMaxUs = _pubReadMaxUs;
    s.busRecoveries = _pubRecoveries;
    portEXIT_CRITICAL(&_mux);
    return s;
}

void Encoder::resetPeriodStats() {
    portENTER_CRITICAL(&_mux);
    _pubReadMaxUs = 0;
    portEXIT_CRITICAL(&_mux);
}
