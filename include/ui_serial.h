#pragma once
#include <Arduino.h>
#include "shared_protocol.h"

void setupUiLink();
void updateUiLink();

bool sendSetModeCommand(MicromouseMode mode);
bool sendStopCommand();
bool requestStatus(MicromouseMode& modeOut);