#include "ODriveCAN.h"
#include "Telemetry.h"
#include <cstring>

ODriveCAN::Channel ODriveCAN::makeChannel(uint8_t nodeId) {
    Channel c{};
    c.nodeId = nodeId;
    c.online = false;
    c.positionValid = false;
    c.currentPosition = 0.0f;
    c.velEstimate = 0.0f;
    c.positionTimestampMs = 0;
    c.lastValidFrameMs = 0;
    c.diagnosticsTimestampMs = 0;
    c.axisError = 0;
    c.axisState = 0;
    c.motorError = 0;
    c.encoderError = 0;
    c.controllerError = 0;
    c.trajectoryDone = false;
    c.iq = 0.0f;
    c.busVoltage = 0.0f;
    c.busCurrent = 0.0f;
    c.txCount = 0;
    c.rxCount = 0;
    c.rxFailCount = 0;
    c.lastTxCommand = 0;
    c.lastRxCommand = 0;
    return c;
}

ODriveCAN::ODriveCAN(Telemetry* telemetry)
    : telemetry_(telemetry),
      right_(makeChannel(RIGHT_NODE_ID)),
      left_(makeChannel(LEFT_NODE_ID)),
      initialized_(false) {}

void ODriveCAN::setTelemetry(Telemetry* telemetry) { telemetry_ = telemetry; }

bool ODriveCAN::begin() {
    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&general, &timing, &filter);
    if (err != ESP_OK) {
        if (telemetry_) telemetry_->log(LogLevel::CRITICAL, "ODriveCAN", "TWAI driver install failed");
        return false;
    }

    err = twai_start();
    if (err != ESP_OK) {
        if (telemetry_) telemetry_->log(LogLevel::CRITICAL, "ODriveCAN", "TWAI start failed");
        return false;
    }

    initialized_ = true;
    return true;
}

void ODriveCAN::update() {
    if (!initialized_) {
        return;
    }

    twai_message_t message{};
    while (readFrame(message)) {
        processFrame(message);
    }

    updateOnlineStates(millis());
}

void ODriveCAN::updateConfigure() {
    // Runtime ASCII/UART configuration is explicitly disabled by the specification.
}

uint32_t ODriveCAN::makeCanId(uint8_t nodeId, uint8_t command) {
    return (static_cast<uint32_t>(nodeId) << 5) | command;
}

bool ODriveCAN::idMatches(uint32_t id, uint8_t nodeId, uint8_t command) {
    return id == makeCanId(nodeId, command);
}

bool ODriveCAN::readFrame(twai_message_t& message) {
    return twai_receive(&message, 0) == ESP_OK;
}

ODriveCAN::Channel* ODriveCAN::channelForNode(uint8_t nodeId) {
    if (nodeId == RIGHT_NODE_ID) return &right_;
    if (nodeId == LEFT_NODE_ID) return &left_;
    return nullptr;
}

void ODriveCAN::markValid(Channel& channel, uint8_t command, uint32_t now) {
    const bool wasOnline = channel.online;
    channel.online = true;
    channel.lastValidFrameMs = now;
    channel.rxCount++;
    channel.lastRxCommand = command;
    if (!wasOnline) {
        logOnline(channel, channel.nodeId == RIGHT_NODE_ID ? "RIGHT" : "LEFT");
    }
}

void ODriveCAN::processFrame(const twai_message_t& message) {
    if (message.extd || message.rtr || message.data_length_code > 8) {
        return;
    }

    const uint8_t nodeId = static_cast<uint8_t>((message.identifier >> 5) & 0x3F);
    const uint8_t command = static_cast<uint8_t>(message.identifier & 0x1F);
    Channel* channel = channelForNode(nodeId);
    if (!channel) {
        return;
    }

    const uint32_t now = millis();
    bool valid = false;

    if (command == CMD_HEARTBEAT && message.data_length_code >= 8) {
        channel->axisError = static_cast<uint32_t>(message.data[0]) |
                             (static_cast<uint32_t>(message.data[1]) << 8) |
                             (static_cast<uint32_t>(message.data[2]) << 16) |
                             (static_cast<uint32_t>(message.data[3]) << 24);
        channel->axisState = message.data[4];
        channel->trajectoryDone = (message.data[5] & 0x08U) != 0;
        valid = true;
    } else if (command == CMD_MOTOR_ERROR && message.data_length_code >= 4) {
        channel->motorError = static_cast<uint32_t>(message.data[0]) |
                              (static_cast<uint32_t>(message.data[1]) << 8) |
                              (static_cast<uint32_t>(message.data[2]) << 16) |
                              (static_cast<uint32_t>(message.data[3]) << 24);
        valid = true;
    } else if (command == CMD_ENCODER_ERROR && message.data_length_code >= 4) {
        channel->encoderError = static_cast<uint32_t>(message.data[0]) |
                                (static_cast<uint32_t>(message.data[1]) << 8) |
                                (static_cast<uint32_t>(message.data[2]) << 16) |
                                (static_cast<uint32_t>(message.data[3]) << 24);
        valid = true;
    } else if (command == CMD_ENCODER_ESTIMATES && message.data_length_code >= 8) {
        float pos = 0.0f;
        float vel = 0.0f;
        memcpy(&pos, &message.data[0], sizeof(float));
        memcpy(&vel, &message.data[4], sizeof(float));
        channel->currentPosition = pos;
        channel->velEstimate = vel;
        channel->positionValid = true;
        channel->positionTimestampMs = now;
        valid = true;
    } else if (command == CMD_GET_IQ && message.data_length_code >= 8) {
        float iqSetpoint = 0.0f;
        float iqMeasured = 0.0f;
        memcpy(&iqSetpoint, &message.data[0], sizeof(float));
        memcpy(&iqMeasured, &message.data[4], sizeof(float));
        (void)iqSetpoint;
        channel->iq = iqMeasured;
        valid = true;
    } else if (command == CMD_GET_BUS_VOLTAGE_CURRENT && message.data_length_code >= 8) {
        memcpy(&channel->busVoltage, &message.data[0], sizeof(float));
        memcpy(&channel->busCurrent, &message.data[4], sizeof(float));
        valid = true;
    } else if (command == CMD_CONTROLLER_ERROR && message.data_length_code >= 4) {
        channel->controllerError = static_cast<uint32_t>(message.data[0]) |
                                   (static_cast<uint32_t>(message.data[1]) << 8) |
                                   (static_cast<uint32_t>(message.data[2]) << 16) |
                                   (static_cast<uint32_t>(message.data[3]) << 24);
        valid = true;
    }

    if (valid) {
        markValid(*channel, command, now);
        channel->diagnosticsTimestampMs = now;
    }
}

void ODriveCAN::updateOnlineStates(uint32_t now) {
    Channel* channels[2] = {&right_, &left_};
    const char* names[2] = {"RIGHT", "LEFT"};

    for (int i = 0; i < 2; ++i) {
        Channel& channel = *channels[i];
        if (channel.online && static_cast<uint32_t>(now - channel.lastValidFrameMs) > CAN_NODE_STALE_MS) {
            channel.online = false;
            logOffline(channel, names[i]);
        }

        if (channel.positionValid && static_cast<uint32_t>(now - channel.positionTimestampMs) > CAN_NODE_STALE_MS) {
            channel.positionValid = false;
        }
    }
}

bool ODriveCAN::sendFrame(uint32_t canId, const uint8_t* data, uint8_t len, Channel& channel, uint8_t command) {
    if (!initialized_ || len > 8) {
        channel.rxFailCount++;
        return false;
    }

    twai_message_t message{};
    message.identifier = canId;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = len;
    memcpy(message.data, data, len);

    const esp_err_t err = twai_transmit(&message, 0);
    if (err != ESP_OK) {
        channel.rxFailCount++;
        logRxError(channel.nodeId == RIGHT_NODE_ID ? "RIGHT CAN transmit failed" : "LEFT CAN transmit failed");
        return false;
    }

    channel.txCount++;
    channel.lastTxCommand = command;
    return true;
}

bool ODriveCAN::sendSetInputPos(Channel& channel, float newPosition) {
    uint8_t data[8] = {};
    memcpy(&data[0], &newPosition, sizeof(float));
    // vel_ff and torque_ff are left at zero.
    return sendFrame(makeCanId(channel.nodeId, CMD_SET_INPUT_POS), data, sizeof(data), channel, CMD_SET_INPUT_POS);
}

bool ODriveCAN::moveRight(float wheelDelta) {
    if (!right_.positionValid) return false;
    const float newPosition = right_.currentPosition + wheelDelta;
    return sendSetInputPos(right_, newPosition);
}

bool ODriveCAN::moveLeft(float wheelDelta) {
    if (!left_.positionValid) return false;
    const float newPosition = left_.currentPosition + wheelDelta;
    return sendSetInputPos(left_, newPosition);
}

bool ODriveCAN::getRightSnapshot(OdriveSnapshot& snapshot) const {
    snapshot = getSnapshotRight();
    return true;
}

bool ODriveCAN::getLeftSnapshot(OdriveSnapshot& snapshot) const {
    snapshot = getSnapshotLeft();
    return true;
}

OdriveSnapshot ODriveCAN::getSnapshotRight() const {
    const Channel& c = right_;
    return {c.online, c.axisState, c.axisError, c.motorError, c.controllerError, c.encoderError,
            c.trajectoryDone, c.iq, c.busVoltage, c.busCurrent, c.velEstimate, c.currentPosition,
            c.positionValid, c.positionTimestampMs, c.diagnosticsTimestampMs, c.txCount, c.rxCount,
            c.rxFailCount, c.lastTxCommand, c.lastRxCommand};
}

OdriveSnapshot ODriveCAN::getSnapshotLeft() const {
    const Channel& c = left_;
    return {c.online, c.axisState, c.axisError, c.motorError, c.controllerError, c.encoderError,
            c.trajectoryDone, c.iq, c.busVoltage, c.busCurrent, c.velEstimate, c.currentPosition,
            c.positionValid, c.positionTimestampMs, c.diagnosticsTimestampMs, c.txCount, c.rxCount,
            c.rxFailCount, c.lastTxCommand, c.lastRxCommand};
}

bool ODriveCAN::rightPositionValid() const { return right_.positionValid; }
bool ODriveCAN::leftPositionValid() const { return left_.positionValid; }
float ODriveCAN::rightCurrentPosition() const { return right_.currentPosition; }
float ODriveCAN::leftCurrentPosition() const { return left_.currentPosition; }

void ODriveCAN::logOffline(Channel& channel, const char* name) {
    if (telemetry_) telemetry_->log(LogLevel::WARNING, "ODriveCAN", name == nullptr ? "ODrive offline" : (String(name) + " ODrive offline").c_str());
}

void ODriveCAN::logOnline(Channel& channel, const char* name) {
    if (telemetry_) telemetry_->log(LogLevel::INFO, "ODriveCAN", name == nullptr ? "ODrive online" : (String(name) + " ODrive online").c_str());
}

void ODriveCAN::logRxError(const char* message) {
    if (telemetry_) telemetry_->log(LogLevel::ERROR, "ODriveCAN", message);
}
