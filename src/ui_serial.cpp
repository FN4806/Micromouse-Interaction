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

// Incrementing sequence number to match replies to requests.
static uint8_t gNextSeq = 1;

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
            gReplySeq = seq;
            gReplyError = ERR_NONE;
            break;

        case MSG_NAK:
            gReplyReceived = true;
            gReplyWasAck = false;
            gReplyWasStatus = false;
            gReplySeq = seq;
            gReplyError = (size >= 3) ? buffer[2] : ERR_INVALID_PAYLOAD;
            break;

        case MSG_STATUS:
            if (size >= 3) {
                gReplyReceived = true;
                gReplyWasAck = false;
                gReplyWasStatus = true;
                gReplySeq = seq;
                gReplyStatusMode = buffer[2];
                gReplyError = ERR_NONE;
            }
            break;

        default:
            // Ignore unknown packets.
            break;
    }
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