#ifndef UI_SERIAL_H
#define UI_SERIAL_H

#include <Arduino.h>
#include "shared_protocol.h"

struct UiSettings {
    uint8_t volume;
    uint8_t startup_music;
    uint8_t combat_music;
};

struct Thresholds {
    uint16_t fl;
    uint16_t fr;
    uint16_t rl;
    uint16_t rr;
};

extern UiSettings ui_settings;
extern Thresholds thresholds;

void setupUiLink();
void updateUiLink();

bool sendSetModeCommand(MicromouseMode mode);
bool sendStopCommand();
bool requestStatus(MicromouseMode& modeOut);

bool saveSetting(uint8_t settingId, uint8_t value);
bool loadSetting(uint8_t settingId, uint8_t& valueOut);
bool loadAllSettings();
void loadDefaultSettings();
bool loadThresholds();
bool calibrateBlack();
bool calibrateWhite();

#endif