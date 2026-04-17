#ifndef SETTINGS_H
#define SETTINGS_H

#include "Arduino.h"

namespace settings {
    // ---------- Program Settings ---------- //
    extern uint8_t ui_volume;
    extern uint8_t startup_sound;
    extern uint8_t combat_music;


    // ---------- DFPlayer Mini Settings ---------- //
    /// @brief DF Player Mini Serial Connection, named as transmit as Pico -> DF Player
    const uint8_t df_tx{0}; 

    /// @brief  DF Player Mini Serial Connection, named as recieve as Pico <- DF Player
    const uint8_t df_rx{1};

    /// @brief DF Player Mini Serial Connection, agreed baud rate
    const uint16_t df_baud{9600};


    // ---------- OLED Settings ---------- //

}

namespace flags {
    extern bool volume_change;
}

#endif