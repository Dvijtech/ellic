#include "ODriveCAN.h"
#include "Telemetry.h"
#include "driver/twai.h"
#include <string.h>

namespace {
// CAN Simple command ID для fw 0.5.6 (раздел 9, 10.4, 12.0,
// "Итого зафиксировал в спецификации")
constexpr uint8_t CMD_HEARTBEAT             = 0x01;
constexpr uint8_t CMD_GET_MOTOR_ERROR       = 0x03;
constexpr uint8_t CMD_GET_ENCODER_ERROR     = 0x04;
constexpr uint8_t CMD_GET_ENCODER_ESTIMATES = 0x09;
constexpr uint8_t CMD_SET_INPUT_POS         = 0x0C;
constexpr uint8_t CMD_GET_IQ                = 0x14;
constexpr uint8_t CMD_GET_BUS_VI            = 0x17;
constexpr uint8_t CMD_GET_CONTROLLER_ERROR  = 0x1D;

uint32_t readU32(const uint8_t* p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

uint64_t readU64(const uint8_t* p) {
    uint64_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

float readF32(const uint8_t* p) {
    float v;
    memcpy(&v, p, sizeof(v));
    return v;
}
} // namespace

ODriveCAN::ODriveCAN() : _telemetry(nullptr) {
    initChannel(_right, RIGHT_ODRIVE_NODE_ID, "RIGHT");
    initChannel(_left, LEFT_ODRIVE_NODE_ID, "LEFT");
}

void ODriveCAN::initChannel(WheelChannel &ch, uint8_t nodeId, const char* label) {
    ch.nodeId = nodeId;
    ch.label = label;
    ch.online = false;
    ch.everReceivedFrame = false;
    ch.lastFrameMs = 0;
    ch.hasValidPosition = false;
    ch.lastPosEstimateMs = 0;
    ch.posEstimate = 0.0f;
    ch.velEstimate = 0.0f;
    ch.axisState = 0;
    ch.axisError = 0;
    ch.motorError = 0;
    ch.encoderError = 0;
    ch.controllerError = 0;
    ch.trajectoryDone = false;
    ch.Iq = 0.0f;
    ch.busVoltage = 0.0f;
    ch.busCurrent = 0.0f;
    ch.txCount = 0;
    ch.rxCount = 0;
    ch.rxFailCount = 0;
    ch.diagnosticsTimestampMs = 0;
}

void ODriveCAN::setTelemetry(Telemetry* telemetry) {
    _telemetry = telemetry;
}

void ODriveCAN::begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();
}

ODriveCAN::WheelChannel* ODriveCAN::channelForNode(uint8_t nodeId) {
    if (nodeId == _right.nodeId) return &_right;
    if (nodeId == _left.nodeId) return &_left;
    return nullptr;
}

void ODriveCAN::processFrame(uint32_t canId, const uint8_t* data, uint8_t dlc, uint32_t nowMs) {
    uint8_t nodeId = (uint8_t)((canId >> 5) & 0x3F);
    uint8_t cmdId  = (uint8_t)(canId & 0x1F);

    WheelChannel* chPtr = channelForNode(nodeId);
    if (chPtr == nullptr) {
        return; // кадр от неизвестного node_id - игнорируется
    }
    WheelChannel &ch = *chPtr;

    bool wasOffline = !ch.online;
    ch.everReceivedFrame = true;
    ch.lastFrameMs = nowMs;
    ch.online = true;
    ch.rxCount++;

    if (wasOffline && _telemetry != nullptr) {
        _telemetry->log(LogLevel::INFO, ch.label, "ODrive CAN: online");
    }

    switch (cmdId) {
        case CMD_HEARTBEAT:
            // Раздел 10.4: Axis_Error(u32 @0) Axis_State(u8 @4)
            // Motor/Encoder/Controller_Error_Flag(бит @5.0/6.0/7.0) Trajectory_Done_Flag(бит @7.7)
            if (dlc >= 8) {
                ch.axisError = readU32(&data[0]);
                ch.axisState = data[4];
                ch.trajectoryDone = (data[7] & 0x80) != 0;
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_MOTOR_ERROR:
            // Внимание: у fw 0.5.6 это поле 64-битное (см. отчёт после кода).
            if (dlc >= 8) {
                ch.motorError = readU64(&data[0]);
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_ENCODER_ERROR:
            if (dlc >= 4) {
                ch.encoderError = readU32(&data[0]);
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_ENCODER_ESTIMATES:
            if (dlc >= 8) {
                ch.posEstimate = readF32(&data[0]);
                ch.velEstimate = readF32(&data[4]);
                ch.hasValidPosition = true;
                ch.lastPosEstimateMs = nowMs;
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_IQ:
            // байты 0..3 - Iq_Setpoint (не используется, "Вопрос 1" спецификации),
            // байты 4..7 - Iq_Measured (используется как фактический ток мотора).
            if (dlc >= 8) {
                ch.Iq = readF32(&data[4]);
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_BUS_VI:
            if (dlc >= 8) {
                ch.busVoltage = readF32(&data[0]);
                ch.busCurrent = readF32(&data[4]);
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        case CMD_GET_CONTROLLER_ERROR:
            if (dlc >= 4) {
                ch.controllerError = readU32(&data[0]);
                ch.diagnosticsTimestampMs = nowMs;
            }
            break;

        default:
            // Прочие CAN Simple сообщения (Get_Version, Encoder Count и т.п.)
            // системой не используются и игнорируются.
            break;
    }
}

void ODriveCAN::refreshOnlineState(uint32_t nowMs) {
    WheelChannel* channels[2] = { &_right, &_left };
    for (WheelChannel* chPtr : channels) {
        WheelChannel &ch = *chPtr;
        if (ch.online && ch.everReceivedFrame &&
            (nowMs - ch.lastFrameMs) > CAN_NODE_STALE_MS) {
            ch.online = false;
            if (_telemetry != nullptr) {
                _telemetry->log(LogLevel::WARNING, ch.label, "ODrive CAN: offline (stale)");
            }
        }
    }
}

void ODriveCAN::update() {
    twai_message_t msg;
    // Раздел 14.1/14.3: приём всех доступных кадров без блокировки.
    while (twai_receive(&msg, 0) == ESP_OK) {
        if (!msg.rtr) {
            processFrame(msg.identifier, msg.data, msg.data_length_code, millis());
        }
    }
    refreshOnlineState(millis());
}

bool ODriveCAN::moveWheelInternal(WheelChannel &ch, float delta) {
    uint32_t now = millis();

    // Раздел 9/13: если текущая позиция недоступна или устарела -
    // команда для этого колеса в этом цикле не отправляется.
    if (!ch.hasValidPosition || (now - ch.lastPosEstimateMs) > CAN_NODE_STALE_MS) {
        ch.rxFailCount++;
        return false;
    }

    float newPosition = ch.posEstimate + delta;

    twai_message_t msg = {};
    msg.identifier = ((uint32_t)ch.nodeId << 5) | CMD_SET_INPUT_POS;
    msg.data_length_code = 8;
    memcpy(&msg.data[0], &newPosition, sizeof(float));
    int16_t velFF = 0;     // раздел 9: move_incremental не используется;
    int16_t torqueFF = 0;  // Vel_FF/Torque_FF спецификацией не заданы -> 0.
    memcpy(&msg.data[4], &velFF, sizeof(velFF));
    memcpy(&msg.data[6], &torqueFF, sizeof(torqueFF));

    if (twai_transmit(&msg, 0) == ESP_OK) {
        ch.txCount++;
        return true;
    }

    if (_telemetry != nullptr) {
        _telemetry->log(LogLevel::WARNING, ch.label, "ODrive CAN: Set Input Pos TX failed");
    }
    return false;
}

bool ODriveCAN::moveLeftWheel(float delta) {
    return moveWheelInternal(_left, delta);
}

bool ODriveCAN::moveRightWheel(float delta) {
    return moveWheelInternal(_right, delta);
}

OdriveSnapshot ODriveCAN::snapshotFrom(const WheelChannel &ch) const {
    OdriveSnapshot s;
    s.online = ch.online;
    s.axisState = ch.axisState;
    s.axisError = ch.axisError;
    s.motorError = ch.motorError;
    s.encoderError = ch.encoderError;
    s.controllerError = ch.controllerError;
    s.trajectoryDone = ch.trajectoryDone;
    s.Iq = ch.Iq;
    s.velEstimate = ch.velEstimate;
    s.busVoltage = ch.busVoltage;
    s.busCurrent = ch.busCurrent;
    s.txCount = ch.txCount;
    s.rxCount = ch.rxCount;
    s.rxFailCount = ch.rxFailCount;
    s.diagnosticsTimestampMs = ch.diagnosticsTimestampMs;
    return s;
}

OdriveSnapshot ODriveCAN::getLeftSnapshot() const {
    return snapshotFrom(_left);
}

OdriveSnapshot ODriveCAN::getRightSnapshot() const {
    return snapshotFrom(_right);
}
