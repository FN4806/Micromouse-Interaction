#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "menu_data.h"

enum ErrorStates {
  DFPLAYER_INIT_FAIL = 0,
  SERIAL_MSG_FAIL
};

namespace display {
    void setup();

    void showMenu(MenuState current_menu, int current_options);
    void showError(ErrorStates error_type);
    void showVolume(uint8_t volume);
    void showThresholds();
    void showModeFace(int current_option);
    void showLoading();
};

#endif