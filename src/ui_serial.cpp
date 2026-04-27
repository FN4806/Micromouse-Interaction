#include "ui_serial.h"
#include <PacketSerial.h>

static COBSPacketSerial packetSerial;

// Track the current request/reply state.
static volatile bool gReplyReceived = false;
static volatile bool gReplyWasAck = false;
static volatile bool gReplyWasStatus = false;
static volatile uint8_t gReplySeq = 0;
static volatile uint8_t gReplyError = ERR_NONE;
static volatile uint8_t gReplyStatusMode = MODE_UI;

static volatile bool gReplyWasSettingValue = false;
static volatile bool gReplyWasAllSettings = false;
static volatile bool gReplyWasThresholds = false;

static volatile uint8_t gReplySettingId = 0;
static volatile uint8_t gReplySettingValue = 0;

static volatile uint8_t gReplyAllStartupMusic = 0;
static volatile uint8_t gReplyAllCombatMusic = 0;
static volatile uint8_t gReplyAllVolume = 0;

// Incrementing sequence number to match replies to requests.
static uint8_t gNextSeq = 1;

UiSettings ui_settings;
Thresholds thresholds;

// ---------- Packet handler ----------
static void onPacketReceived(const uint8_t* buffer, size_t size)
{
    if (size < 2) {
        return; // Need at least [type, seq]
    }

    const uint8_t type = buffer[0];
    const uint8_t seq  = buffer[1];

    switch (type) {
        case MSG_ACK:
            gReplyReceived = true;
            gReplyWasAck = true;
            gReplyWasStatus = false;
            gReplyWasSettingValue = false;
            gReplyWasAllSettings = false;
            gReplyWasThresholds = false;
            gReplySeq = seq;
            gReplyError = ERR_NONE;
            break;

        case MSG_NAK:
            gReplyReceived = true;
            gReplyWasAck = false;
            gReplyWasStatus = false;
            gReplyWasSettingValue = false;
            gReplyWasAllSettings = false;
            gReplyWasThresholds = false;
            gReplySeq = seq;
            gReplyError = (size >= 3) ? buffer[2] : ERR_INVALID_PAYLOAD;
            break;

        case MSG_STATUS:
            if (size >= 3) {
                gReplyReceived = true;
                gReplyWasAck = false;
                gReplyWasStatus = true;
                gReplyWasSettingValue = false;
                gReplyWasAllSettings = false;
                gReplyWasThresholds = false;
                gReplySeq = seq;
                gReplyStatusMode = buffer[2];
                gReplyError = ERR_NONE;
            }
            break;

        case MSG_SETTING_VALUE:
            if (size >= 4) {
                gReplyReceived = true;
                gReplyWasAck = false;
                gReplyWasStatus = false;
                gReplyWasSettingValue = true;
                gReplyWasAllSettings = false;
                gReplyWasThresholds = false;
                gReplySeq = seq;
                gReplySettingId = buffer[2];
                gReplySettingValue = buffer[3];
                gReplyError = ERR_NONE;
            }
            break;

        case MSG_ALL_SETTINGS:
            if (size >= 5) {
                gReplyReceived = true;
                gReplyWasAck = false;
                gReplyWasStatus = false;
                gReplyWasSettingValue = false;
                gReplyWasAllSettings = true;
                gReplyWasThresholds = false;
                gReplySeq = seq;
                gReplyAllStartupMusic = buffer[2];
                gReplyAllCombatMusic = buffer[3];
                gReplyAllVolume = buffer[4];
                gReplyError = ERR_NONE;
            }
            break;

        case MSG_THRESHOLDS:
            if (size == 10) {
                gReplyReceived = true;

                gReplyWasAck = false;
                gReplyWasStatus = false;
                gReplyWasSettingValue = false;
                gReplyWasAllSettings = false;
                gReplyWasThresholds = true;

                gReplySeq = seq;
                
                // Static cast to uint16_t since buffer is originally uint8_t but we need room to shift 
                thresholds.fl = (static_cast<uint16_t>(buffer[2]) << 8) | buffer[3];
                thresholds.fr = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
                thresholds.rl = (static_cast<uint16_t>(buffer[6]) << 8) | buffer[7];
                thresholds.rr = (static_cast<uint16_t>(buffer[8]) << 8) | buffer[9];
            }
            break;

        default:
            break;
    }
}

static void clearReplyState()
{
    gReplyReceived = false;
    gReplyWasAck = false;
    gReplyWasStatus = false;
    gReplyWasSettingValue = false;
    gReplyWasAllSettings = false;
    gReplyWasThresholds = false;

    gReplySeq = 0;
    gReplyError = ERR_NONE;

    gReplySettingId = 0;
    gReplySettingValue = 0;

    gReplyAllStartupMusic = 0;
    gReplyAllCombatMusic = 0;
    gReplyAllVolume = 0;
}

void setupUiLink()
{
    // Inter-Pico UART.
    Serial2.setTX(8);   // Change if your chosen pins differ
    Serial2.setRX(9);   // Change if your chosen pins differ
    Serial2.begin(LINK_BAUD);

    packetSerial.setStream(&Serial2);
    packetSerial.setPacketHandler(&onPacketReceived);

    Serial.println("UI link ready");
}

void updateUiLink()
{
    packetSerial.update();

    if (packetSerial.overflow()) {
        Serial.println("UI link receive overflow");
    }
}

// Wait for ACK / NAK / STATUS matching a specific sequence byte.
static bool waitForReply(uint8_t expectedSeq, uint32_t timeoutMs, bool expectStatus, MicromouseMode* modeOut)
{
    const unsigned long start = millis();

    while (millis() - start < timeoutMs) {
        updateUiLink();

        if (!gReplyReceived) {
            continue;
        }

        // Consume only matching replies.
        if (gReplySeq != expectedSeq) {
            gReplyReceived = false;
            continue;
        }

        if (expectStatus) {
            if (gReplyWasStatus) {
                if (modeOut) {
                    *modeOut = static_cast<MicromouseMode>(gReplyStatusMode);
                }
                gReplyReceived = false;
                return true;
            }

            // NAK for request-status counts as failure.
            if (!gReplyWasAck && !gReplyWasStatus) {
                Serial.print("Status request failed, error=");
                Serial.println(gReplyError);
                gReplyReceived = false;
                return false;
            }
        } else {
            if (gReplyWasAck) {
                gReplyReceived = false;
                return true;
            }

            if (!gReplyWasAck) {
                Serial.print("Command NAK, error=");
                Serial.println(gReplyError);
                gReplyReceived = false;
                return false;
            }
        }
    }

    Serial.println("Reply timeout");
    return false;
}

static bool sendCommandWithRetry(const uint8_t* packet, size_t size, uint8_t seq, bool expectStatus, MicromouseMode* modeOut = nullptr)
{
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        gReplyReceived = false;
        gReplyWasAck = false;
        gReplyWasStatus = false;
        gReplyError = ERR_NONE;
        gReplySeq = 0;

        packetSerial.send(packet, size);

        if (waitForReply(seq, REPLY_TIMEOUT_MS, expectStatus, modeOut)) {
            return true;
        }

        delay(20);
    }

    return false;
}

bool sendSetModeCommand(MicromouseMode mode)
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[3];
    packet[0] = MSG_CMD_SET_MODE;
    packet[1] = seq;
    packet[2] = static_cast<uint8_t>(mode);

    return sendCommandWithRetry(packet, sizeof(packet), seq, false);
}

bool sendStopCommand()
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_STOP;
    packet[1] = seq;

    return sendCommandWithRetry(packet, sizeof(packet), seq, false);
}

bool requestStatus(MicromouseMode& modeOut)
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_REQUEST_STATUS;
    packet[1] = seq;

    return sendCommandWithRetry(packet, sizeof(packet), seq, true, &modeOut);
}

bool saveSetting(uint8_t settingId, uint8_t value)
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[4];
    packet[0] = MSG_CMD_WRITE_SETTING;
    packet[1] = seq;
    packet[2] = settingId;
    packet[3] = value;

    return sendCommandWithRetry(packet, sizeof(packet), seq, false);
}

bool loadSetting(uint8_t settingId, uint8_t& valueOut)
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[3];
    packet[0] = MSG_CMD_READ_SETTING;
    packet[1] = seq;
    packet[2] = settingId;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        clearReplyState();

        packetSerial.send(packet, sizeof(packet));

        const unsigned long start = millis();
        while (millis() - start < REPLY_TIMEOUT_MS) {
            updateUiLink();

            if (!gReplyReceived) {
                continue;
            }

            if (gReplySeq != seq) {
                clearReplyState();
                continue;
            }

            if (gReplyWasSettingValue) {
                if (gReplySettingId == settingId) {
                    valueOut = gReplySettingValue;
                    clearReplyState();
                    return true;
                } else {
                    clearReplyState();
                    return false;
                }
            }

            if (!gReplyWasAck && !gReplyWasStatus && !gReplyWasAllSettings && !gReplyWasSettingValue && !gReplyWasThresholds) {
                clearReplyState();
                return false;
            }
        }

        delay(20);
    }

    return false;
}

bool loadAllSettings()
{
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_READ_ALL_SETTINGS;
    packet[1] = seq;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        clearReplyState();

        packetSerial.send(packet, sizeof(packet));

        const unsigned long start = millis();
        while (millis() - start < REPLY_TIMEOUT_MS) {
            updateUiLink();

            if (!gReplyReceived) {
                continue;
            }

            if (gReplySeq != seq) {
                clearReplyState();
                continue;
            }

            if (gReplyWasAllSettings) {
                ui_settings.startup_music   = gReplyAllStartupMusic;
                ui_settings.combat_music    = gReplyAllCombatMusic;
                ui_settings.volume          = gReplyAllVolume;

                clearReplyState();
                return true;
            }

            if (!gReplyWasAck && !gReplyWasStatus && !gReplyWasSettingValue && !gReplyWasAllSettings && !gReplyWasThresholds) {
                clearReplyState();
                return false;
            }
        }

        delay(20);
    }

    return false;
}

void loadDefaultSettings() {
    ui_settings.startup_music   = 5;
    ui_settings.combat_music    = 3;
    ui_settings.volume          = 20;
}

bool loadThresholds() {
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_REQUEST_THRESHOLDS;
    packet[1] = seq;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        clearReplyState();

        packetSerial.send(packet, sizeof(packet));

        const unsigned long start = millis();
        while (millis() - start < REPLY_TIMEOUT_MS) {
            updateUiLink();

            if (!gReplyReceived) {
                continue;
            }

            if (gReplySeq != seq) {
                clearReplyState();
                continue;
            }

            if (gReplyWasThresholds) {
                clearReplyState();
                return true;
            }

            if (!gReplyWasAck && !gReplyWasStatus && !gReplyWasAllSettings && !gReplyWasSettingValue && !gReplyWasThresholds) {
                clearReplyState();
                return false;
            }
        }

        delay(20);
    }

    return false;
}

bool calibrateWhite() {
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_CALIBRATE_WHITE;
    packet[1] = seq;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        clearReplyState();

        packetSerial.send(packet, sizeof(packet));

        const unsigned long start = millis();
        while (millis() - start < REPLY_TIMEOUT_MS) {
            updateUiLink();

            if (!gReplyReceived) {
                continue;
            }

            if (gReplySeq != seq) {
                clearReplyState();
                continue;
            }

            if (gReplyWasThresholds) {
                clearReplyState();
                return true;
            }

            if (!gReplyWasAck && !gReplyWasStatus && !gReplyWasAllSettings && !gReplyWasSettingValue && !gReplyWasThresholds) {
                clearReplyState();
                return false;
            }
        }

        delay(20);
    }

    return false;
}

bool calibrateBlack() {
    const uint8_t seq = gNextSeq++;

    uint8_t packet[2];
    packet[0] = MSG_CMD_CALIBRATE_BLACK;
    packet[1] = seq;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        clearReplyState();

        packetSerial.send(packet, sizeof(packet));

        const unsigned long start = millis();
        while (millis() - start < REPLY_TIMEOUT_MS) {
            updateUiLink();

            if (!gReplyReceived) {
                continue;
            }

            if (gReplySeq != seq) {
                clearReplyState();
                continue;
            }

            if (gReplyWasThresholds) {
                clearReplyState();
                return true;
            }

            if (!gReplyWasAck && !gReplyWasStatus && !gReplyWasAllSettings && !gReplyWasSettingValue && !gReplyWasThresholds) {
                clearReplyState();
                return false;
            }
        }

        delay(20);
    }

    return false;
}