#ifndef UI_SERIAL_H
#define UI_SERIAL_H

#include <Arduino.h>
#include "shared_protocol.h"

struct UiSettings {
    uint8_t volume;
    uint8_t startup_music;
    uint8_t combat_music;
};

extern UiSettings ui_settings;

void setupUiLink();
void updateUiLink();

bool sendSetModeCommand(MicromouseMode mode);
bool sendStopCommand();
bool requestStatus(MicromouseMode& modeOut);

bool saveSetting(uint8_t settingId, uint8_t value);
bool loadSetting(uint8_t settingId, uint8_t& valueOut);
bool loadAllSettings();
void loadDefaultSettings();

#endif