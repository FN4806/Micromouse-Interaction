#pragma once
#include <Arduino.h>

// ---------- Message types ----------
enum MessageType : uint8_t {
    MSG_CMD_SET_MODE            = 0x01,
    MSG_CMD_STOP                = 0x02,
    MSG_CMD_REQUEST_STATUS      = 0x03,

    MSG_CMD_WRITE_SETTING       = 0x10,
    MSG_CMD_READ_SETTING        = 0x11,
    MSG_CMD_READ_ALL_SETTINGS   = 0x12,
    MSG_CMD_REQUEST_THRESHOLDS  = 0x13,
    MSG_CMD_CALIBRATE_WHITE     = 0x14,
    MSG_CMD_CALIBRATE_BLACK     = 0x15,

    MSG_ACK                     = 0x80,
    MSG_NAK                     = 0x81,
    MSG_STATUS                  = 0x82,
    MSG_SETTING_VALUE           = 0x83,
    MSG_ALL_SETTINGS            = 0x84,
    MSG_THRESHOLDS              = 0x85
    
};

// ---------- Robot modes ----------
enum MicromouseMode : uint8_t {
    MODE_UI = 0,
    MODE_LINE_FOLLOWING,
    MODE_COMBAT,
    MODE_OBSTACLE_AVOIDANCE
};

// ---------- Error codes ----------
enum ErrorCode : uint8_t {
    ERR_NONE = 0,
    ERR_INVALID_CMD = 1,
    ERR_INVALID_PAYLOAD = 2,
    ERR_BUSY = 3,
    ERR_INVALID_SETTING = 4,
    ERR_STORAGE_FAIL = 5
};

enum SettingId : uint8_t {
    SETTING_STARTUP_MUSIC = 0x01,
    SETTING_COMBAT_MUSIC = 0x02,
    SETTING_VOLUME = 0x03
};

// ---------- Packet layout ----------
// Command packet:
//   [0] = message type
//   [1] = sequence number
//   [2...] = payload
//
// Replies:
// ACK:
//   [0] = MSG_ACK
//   [1] = sequence number
//
// NAK:
//   [0] = MSG_NAK
//   [1] = sequence number
//   [2] = error code
//
// STATUS:
//   [0] = MSG_STATUS
//   [1] = sequence number
//   [2] = current mode

constexpr uint32_t LINK_BAUD = 115200;
constexpr uint32_t REPLY_TIMEOUT_MS = 200;
constexpr uint8_t MAX_RETRIES = 3;