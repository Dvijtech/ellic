#include "ODriveUART.h"
#include "Telemetry.h"
#include <math.h>

ODriveUART::ODriveUART(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud, const char* label)
: _serial(serial), _rxPin(rxPin), _txPin(txPin), _baud(baud), _telemetry(nullptr),
  _busy(false), _purpose(Purpose::NONE), _requestStartMs(0), _rxBuffer(""),
  _moveWheelPending(false), _queuedWheelDelta(0.0f), _activeWheelDelta(0.0f),
  _configStage(ConfigStage::NotStarted), _configRequestPending(false),
  _configPendingPurpose(Purpose::NONE), _configAwaitingResponse(false),
  _configResponseReady(false), _configResponseOk(false), _configResponseValue(0.0f),
  _diagIndex(0), _Iq(0.0f), _Vq(0.0f), _velEstimate(0.0f),
  _axisState(0), _axisError(0), _motorError(0), _controllerError(0),
  _diagnosticsTimestampMs(0),
  _online(false), _failStreak(0), _txCount(0), _rxCount(0), _rxFailCount(0)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = 0;
}

void ODriveUART::setTelemetry(Telemetry* telemetry) {
    _telemetry = telemetry;
}

void ODriveUART::begin() {
    _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
}

// ---------- парсинг ----------

bool ODriveUART::parseOneFloat(const String &line, float &out) {
    String t = line;
    t.trim();
    if (t.length() == 0) return false;
    char* endPtr;
    float v = strtod(t.c_str(), &endPtr);
    if (endPtr == t.c_str()) return false;
    out = v;
    return true;
}

bool ODriveUART::parseTwoFloats(const String &line, float &a, float &b) {
    String t = line;
    t.trim();
    int sp = t.indexOf(' ');
    if (sp < 0) return false;
    String s1 = t.substring(0, sp);
    String s2 = t.substring(sp + 1);
    s2.trim();
    char* e1;
    char* e2;
    float v1 = strtod(s1.c_str(), &e1);
    float v2 = strtod(s2.c_str(), &e2);
    if (e1 == s1.c_str() || e2 == s2.c_str()) return false;
    a = v1;
    b = v2;
    return true;
}

// ---------- низкоуровневый обмен ----------

void ODriveUART::sendRequest(const char* cmd, Purpose purpose) {
    _serial.print(cmd);
    _serial.print("\n");
    _txCount++;
    _busy = true;
    _purpose = purpose;
    _requestStartMs = millis();
    _rxBuffer = "";
}

void ODriveUART::onExchangeSuccess() {
    _rxCount++;
    bool wasOffline = !_online;
    _failStreak = 0;
    _online = true;
    if (wasOffline && _telemetry) {
        _telemetry->log(LogLevel::INFO, _label, "ODrive online (connection restored)");
    }
}

void ODriveUART::onExchangeFailure() {
    _rxFailCount++;
    if (_failStreak < 255) _failStreak++;
    if (_failStreak >= ALIVE_FAIL_THRESHOLD && _online) {
        _online = false;
        if (_telemetry) {
            _telemetry->log(LogLevel::WARNING, _label, "ODrive offline (failStreak >= threshold)");
        }
    }
}

void ODriveUART::handleResponseLine(const String &line) {
    switch (_purpose) {
        case Purpose::MOVE_READ_POS: {
            float pos, vel;
            bool ok = parseTwoFloats(line, pos, vel);
            if (ok) {
                onExchangeSuccess();
                float newPos = pos + _activeWheelDelta;
                char buf[32];
                snprintf(buf, sizeof(buf), "p 0 %.4f\n", newPos);
                _serial.print(buf);
                _txCount++;
            } else {
                onExchangeFailure();
                if (_telemetry) {
                    _telemetry->log(LogLevel::WARNING, _label, "f0 read failed, p0 not sent this cycle");
                }
            }
            break;
        }
        case Purpose::CONFIG_CHECK_IDLE:
        case Purpose::CONFIG_CHECK_CLOSEDLOOP: {
            float val;
            bool ok = parseOneFloat(line, val);
            if (ok) onExchangeSuccess(); else onExchangeFailure();
            _configResponseOk = ok;
            _configResponseValue = ok ? val : 0.0f;
            _configResponseReady = true;
            _configAwaitingResponse = false;
            break;
        }
        case Purpose::DIAG_IQ: {
            float v;
            if (parseOneFloat(line, v)) { _Iq = v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 1;
            break;
        }
        case Purpose::DIAG_VQ: {
            float v;
            if (parseOneFloat(line, v)) { _Vq = v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 2;
            break;
        }
        case Purpose::DIAG_VEL: {
            float v;
            if (parseOneFloat(line, v)) { _velEstimate = v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 3;
            break;
        }
        case Purpose::DIAG_STATE: {
            float v;
            if (parseOneFloat(line, v)) { _axisState = (int)v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 4;
            break;
        }
        case Purpose::DIAG_AXIS_ERR: {
            float v;
            if (parseOneFloat(line, v)) { _axisError = (int)v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 5;
            break;
        }
        case Purpose::DIAG_MOTOR_ERR: {
            float v;
            if (parseOneFloat(line, v)) { _motorError = (int)v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 6;
            break;
        }
        case Purpose::DIAG_CTRL_ERR: {
            float v;
            if (parseOneFloat(line, v)) { _controllerError = (int)v; onExchangeSuccess(); _diagnosticsTimestampMs = millis(); }
            else onExchangeFailure();
            _diagIndex = 0;
            break;
        }
        case Purpose::NONE:
        default:
            break;
    }
}

void ODriveUART::handleTimeout() {
    switch (_purpose) {
        case Purpose::MOVE_READ_POS:
            onExchangeFailure();
            if (_telemetry) {
                _telemetry->log(LogLevel::WARNING, _label, "f0 timeout, p0 not sent this cycle");
            }
            break;
        case Purpose::CONFIG_CHECK_IDLE:
        case Purpose::CONFIG_CHECK_CLOSEDLOOP:
            onExchangeFailure();
            _configResponseOk = false;
            _configResponseValue = 0.0f;
            _configResponseReady = true;
            _configAwaitingResponse = false;
            break;
        case Purpose::DIAG_IQ:        onExchangeFailure(); _diagIndex = 1; break;
        case Purpose::DIAG_VQ:        onExchangeFailure(); _diagIndex = 2; break;
        case Purpose::DIAG_VEL:       onExchangeFailure(); _diagIndex = 3; break;
        case Purpose::DIAG_STATE:     onExchangeFailure(); _diagIndex = 4; break;
        case Purpose::DIAG_AXIS_ERR:  onExchangeFailure(); _diagIndex = 5; break;
        case Purpose::DIAG_MOTOR_ERR: onExchangeFailure(); _diagIndex = 6; break;
        case Purpose::DIAG_CTRL_ERR:  onExchangeFailure(); _diagIndex = 0; break;
        case Purpose::NONE:
        default:
            break;
    }
}

void ODriveUART::dispatchNext() {
    // 1) MoveWheel — наивысший приоритет
    if (_moveWheelPending) {
        _moveWheelPending = false;
        _activeWheelDelta = _queuedWheelDelta;
        sendRequest("f 0", Purpose::MOVE_READ_POS);
        return;
    }
    // 2) запросы конечного автомата конфигурации
    if (_configRequestPending) {
        _configRequestPending = false;
        _configAwaitingResponse = true;
        sendRequest("r axis0.current_state", _configPendingPurpose);
        return;
    }
    // 3) фоновая диагностика: current -> voltage -> velocity -> state -> errors -> по кругу
    switch (_diagIndex) {
        case 0: sendRequest("r axis0.motor.current_control.Iq_measured", Purpose::DIAG_IQ); break;
        case 1: sendRequest("r axis0.motor.current_control.Vq_setpoint", Purpose::DIAG_VQ); break;
        case 2: sendRequest("r axis0.encoder.vel_estimate", Purpose::DIAG_VEL); break;
        case 3: sendRequest("r axis0.current_state", Purpose::DIAG_STATE); break;
        case 4: sendRequest("r axis0.error", Purpose::DIAG_AXIS_ERR); break;
        case 5: sendRequest("r axis0.motor.error", Purpose::DIAG_MOTOR_ERR); break;
        default: sendRequest("r axis0.controller.error", Purpose::DIAG_CTRL_ERR); break;
    }
}

void ODriveUART::update() {
    if (_busy) {
        while (_serial.available()) {
            char c = _serial.read();
            if (c == '\n') {
                handleResponseLine(_rxBuffer);
                _rxBuffer = "";
                _busy = false;
                break;
            } else if (c != '\r') {
                _rxBuffer += c;
            }
        }
        if (_busy && (millis() - _requestStartMs >= RESPONSE_TIMEOUT_MS)) {
            handleTimeout();
            _busy = false;
            _rxBuffer = "";
        }
    }

    if (!_busy) {
        dispatchNext();
    }
}

// ---------- requestMove ----------

void ODriveUART::requestMove(float wheelDelta) {
    _moveWheelPending = true;
    _queuedWheelDelta = wheelDelta;
}

// ---------- конфигурация (раздел 10.2) ----------

void ODriveUART::sendConfigCommands() {
    _serial.print("w axis0.controller.config.control_mode 3\n"); _txCount++; delay(20);
    _serial.print("w axis0.controller.config.input_mode 2\n");   _txCount++; delay(20);
    _serial.print("w axis0.trap_traj.config.vel_limit 2.0\n");   _txCount++; delay(20);
    _serial.print("w axis0.trap_traj.config.accel_limit 10.0\n"); _txCount++; delay(20);
    _serial.print("w axis0.trap_traj.config.decel_limit 10.0\n"); _txCount++; delay(20);
    _serial.print("w axis0.requested_state 8\n");                _txCount++; delay(100);
}

void ODriveUART::updateConfigure() {
    switch (_configStage) {
        case ConfigStage::NotStarted:
            if (!_configRequestPending && !_configAwaitingResponse) {
                _configPendingPurpose = Purpose::CONFIG_CHECK_IDLE;
                _configRequestPending = true;
                _configStage = ConfigStage::WaitingIdle;
            }
            break;

        case ConfigStage::WaitingIdle:
            if (_configResponseReady) {
                _configResponseReady = false;
                bool isIdle = _configResponseOk && (fabsf(_configResponseValue - 1.0f) < 0.5f);
                if (isIdle) {
                    _configStage = ConfigStage::Configuring;
                    sendConfigCommands(); // блокирующий вызов, как определено спецификацией
                    _configStage = ConfigStage::ConfirmingClosedLoop;
                    _configPendingPurpose = Purpose::CONFIG_CHECK_CLOSEDLOOP;
                    _configRequestPending = true;
                } else {
                    _configStage = ConfigStage::NotStarted;
                }
            }
            break;

        case ConfigStage::Configuring:
            // Стадия проходится синхронно внутри WaitingIdle, сюда управление не возвращается.
            break;

        case ConfigStage::ConfirmingClosedLoop:
            if (_configResponseReady) {
                _configResponseReady = false;
                bool isClosedLoop = _configResponseOk && (fabsf(_configResponseValue - 8.0f) < 0.5f);
                if (isClosedLoop) {
                    _configStage = ConfigStage::Done;
                } else {
                    _configStage = ConfigStage::NotStarted;
                }
            }
            break;

        case ConfigStage::Done:
            break;
    }
}

// ---------- snapshot ----------

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