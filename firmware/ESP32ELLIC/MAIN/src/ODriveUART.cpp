#include "ODriveUART.h"
#include "Telemetry.h"
#include <string.h>
#include <stdio.h>

ODriveUART::ODriveUART(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, const char* label)
    : _serial(serial),
      _rxPin(rxPin),
      _txPin(txPin),
      _label(label),
      _telemetry(nullptr),
      _configState(ConfigState::NotStarted),
      _confirmRequestSent(false),
      _requestPending(false),
      _requestSentMs(0),
      _rxBufferLen(0),
      _diagIndex(0),
      _lastDiagRequestMs(0),
      _failStreak(0),
      _online(false),
      _txCount(0),
      _rxCount(0),
      _rxFailCount(0),
      _axisState(0),
      _axisError(0),
      _motorError(0),
      _controllerError(0),
      _Iq(0.0f),
      _Vq(0.0f),
      _velEstimate(0.0f),
      _diagnosticsTimestampMs(0) {
    _rxBuffer[0] = '\0';
}

void ODriveUART::setTelemetry(Telemetry* telemetry) {
    _telemetry = telemetry;
}

void ODriveUART::begin() {
    _serial.begin(ODRIVE_UART_BAUD, SERIAL_8N1, _rxPin, _txPin);
}

// =======================================================================
// Низкоуровневый асинхронный обмен (раздел 13.1, 14.2.2)
// =======================================================================

void ODriveUART::flushRx() {
    _rxBufferLen = 0;
    _rxBuffer[0] = '\0';
    while (_serial.available()) {
        _serial.read();
    }
}

void ODriveUART::onSuccess() {
    _rxCount++;
    _failStreak = 0;
    _online = true;
}

void ODriveUART::onFailure() {
    _rxFailCount++;
    _failStreak++;
    if (_failStreak >= ALIVE_FAIL_THRESHOLD) {
        _online = false;
    }
    // Раздел 13.1: очистка программного и физического RX-буфера после ошибки.
    flushRx();
}

void ODriveUART::sendRequestAsync(const char* cmd) {
    // Раздел 14.2.2: перед отправкой UART должен быть синхронизирован.
    flushRx();
    _serial.print(cmd);
    _txCount++;
    _requestPending = true;
    _requestSentMs = millis();
    _rxBufferLen = 0;
    _rxBuffer[0] = '\0';
}

int ODriveUART::pollResponse() {
    if (!_requestPending) {
        return -1;
    }

    while (_serial.available()) {
        char c = (char)_serial.read();
        if (c == '\n') {
            _requestPending = false;
            onSuccess();
            return 1;
        } else if (c != '\r') {
            if (_rxBufferLen < sizeof(_rxBuffer) - 1) {
                _rxBuffer[_rxBufferLen++] = c;
                _rxBuffer[_rxBufferLen] = '\0';
            }
        }
    }

    if (millis() - _requestSentMs >= RESPONSE_TIMEOUT_MS) {
        _requestPending = false;
        onFailure();
        return -1;
    }

    return 0;
}

bool ODriveUART::waitForResponse(char* outBuf, size_t bufSize) {
    while (true) {
        int r = pollResponse();
        if (r == 1) {
            strncpy(outBuf, _rxBuffer, bufSize - 1);
            outBuf[bufSize - 1] = '\0';
            return true;
        }
        if (r == -1) {
            return false;
        }
        // r == 0: продолжаем ждать (без задержки - таймаут проверяется в pollResponse)
    }
}

// =======================================================================
// MoveWheel (раздел 9, 14.2.1)
// =======================================================================

bool ODriveUART::moveWheel(float delta) {
    // Раздел 14.2.3: если сейчас идёт чужой (диагностика/конфигурация) обмен,
    // MoveWheel дожидается его завершения (успех или таймаут RESPONSE_TIMEOUT_MS),
    // но не прерывает и не отменяет его - только после этого начинает f0.
    if (_requestPending) {
        char discard[64];
        waitForResponse(discard, sizeof(discard));
    }

    // f 0 -> currentPosition
    sendRequestAsync("f 0\n");
    char buf[64];
    if (!waitForResponse(buf, sizeof(buf))) {
        if (_telemetry != nullptr) {
            _telemetry->log(LogLevel::WARNING, _label, "MoveWheel: f0 timeout, p0 skipped");
        }
        return false;
    }

    float currentPosition, currentVelocity;
    if (sscanf(buf, "%f %f", &currentPosition, &currentVelocity) != 2) {
        if (_telemetry != nullptr) {
            _telemetry->log(LogLevel::WARNING, _label, "MoveWheel: f0 parse error, p0 skipped");
        }
        return false;
    }

    float newPosition = currentPosition + delta;
    char cmd[40];
    snprintf(cmd, sizeof(cmd), "p 0 %.4f\n", newPosition);
    _serial.print(cmd);
    _txCount++;
    return true;
}

// =======================================================================
// Автоконфигурация (раздел 10.2)
// =======================================================================

void ODriveUART::sendConfigCommands() {
    static const char* cmds[] = {
        "w axis0.controller.config.control_mode 3\n",
        "w axis0.controller.config.input_mode 1\n",
        "w axis0.trap_traj.config.vel_limit 2.0\n",
        "w axis0.trap_traj.config.accel_limit 10.0\n",
        "w axis0.trap_traj.config.decel_limit 10.0\n",
        "w axis0.requested_state 8\n"
    };
    const int n = sizeof(cmds) / sizeof(cmds[0]);
    for (int i = 0; i < n; i++) {
        _serial.print(cmds[i]);
        _txCount++;
        delay(20);
    }
    delay(100);
}

void ODriveUART::updateConfigure() {
    switch (_configState) {
        case ConfigState::NotStarted:
            sendRequestAsync("r axis0.current_state\n");
            _configState = ConfigState::WaitingIdle;
            break;

        case ConfigState::WaitingIdle: {
            int r = pollResponse();
            if (r == 0) {
                return;
            }
            if (r == 1) {
                float state = -1.0f;
                if (sscanf(_rxBuffer, "%f", &state) == 1) {
                    int s = (int)state;
                    if (s == 1 || s == 8) {
                        // IDLE (1) или уже CLOSED_LOOP_CONTROL (8) — оба случая
                        // ведут в Configuring; sendConfigCommands() выполнится
                        // в обоих случаях, см. раздел 10.2 спецификации.
                        _configState = ConfigState::Configuring;
                    } else {
                        _configState = ConfigState::NotStarted;
                    }
                } else {
                    _configState = ConfigState::NotStarted;
                }
            } else {
                _configState = ConfigState::NotStarted;
            }
            break;
        }
        case ConfigState::Configuring:
            sendConfigCommands();
            _confirmRequestSent = false;
            _configState = ConfigState::ConfirmingClosedLoop;
            break;

        case ConfigState::ConfirmingClosedLoop: {
            if (!_confirmRequestSent) {
                sendRequestAsync("r axis0.current_state\n");
                _confirmRequestSent = true;
                return;
            }
            int r = pollResponse();
            if (r == 0) {
                return;
            }
            if (r == 1) {
                float state = -1.0f;
                if (sscanf(_rxBuffer, "%f", &state) == 1 && (int)state == 8) {
                    _configState = ConfigState::Done;
                    if (_telemetry != nullptr) {
                        _telemetry->log(LogLevel::INFO, _label, "CLOSED_LOOP_CONTROL confirmed");
                    }
                } else {
                    _configState = ConfigState::NotStarted;
                }
            } else {
                _configState = ConfigState::NotStarted;
            }
            break;
        }

        case ConfigState::Done:
            // конфигурация подтверждена, updateConfigure() больше ничего не делает
            break;
    }
}

// =======================================================================
// Фоновая диагностика (раздел 12.1)
// =======================================================================

void ODriveUART::applyDiagResult(int index, const char* buf) {
    float fval = 0.0f;
    sscanf(buf, "%f", &fval);

    switch (index) {
        case 0: _Iq = fval; break;                    // Iq_measured
        case 1: _Vq = fval; break;                    // Vq_setpoint
        case 2: _velEstimate = fval; break;            // vel_estimate
        case 3: _axisState = (int)fval; break;         // current_state
        case 4: _axisError = (int)fval; break;         // axis0.error
        case 5: _motorError = (int)fval; break;        // motor.error
        case 6: _controllerError = (int)fval; break;   // controller.error
        default: break;
    }
    _diagnosticsTimestampMs = millis();
}

void ODriveUART::updateDiagnostics() {
    static const char* params[7] = {
        "axis0.motor.current_control.Iq_measured",
        "axis0.motor.current_control.Vq_setpoint",
        "axis0.encoder.vel_estimate",
        "axis0.current_state",
        "axis0.error",
        "axis0.motor.error",
        "axis0.controller.error"
    };

    if (_requestPending) {
        int r = pollResponse();
        if (r == 0) {
            return;
        }
        if (r == 1) {
            applyDiagResult(_diagIndex, _rxBuffer);
        }
        // и при успехе, и при таймауте - переходим к следующему параметру
        // по кругу (раздел 12.1)
        _diagIndex = (_diagIndex + 1) % 7;
        return;
    }

    if (millis() - _lastDiagRequestMs >= DIAG_PERIOD_MS) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "r %s\n", params[_diagIndex]);
        sendRequestAsync(cmd);
        _lastDiagRequestMs = millis();
    }
}

// =======================================================================
// Диспетчер (раздел 14): вызывается раз в loop()
// =======================================================================

void ODriveUART::update() {
    // Приоритет 1 (MoveWheel) обслуживается напрямую из main.cpp через
    // moveWheel(), вне этого метода.
    if (_configState != ConfigState::Done) {
        // Приоритет 2: конфигурация имеет приоритет над диагностикой,
        // пока не завершена.
        updateConfigure();
        return;
    }
    // Приоритет 3: фоновая диагностика.
    updateDiagnostics();
}

OdriveSnapshot ODriveUART::getSnapshot() const {
    OdriveSnapshot s;
    s.online = _online;
    s.axisState = _axisState;
    s.axisError = _axisError;
    s.motorError = _motorError;
    s.controllerError = _controllerError;
    s.Iq = _Iq;
    s.Vq = _Vq;
    s.velEstimate = _velEstimate;
    s.txCount = _txCount;
    s.rxCount = _rxCount;
    s.rxFailCount = _rxFailCount;
    s.diagnosticsTimestampMs = _diagnosticsTimestampMs;
    return s;
}
