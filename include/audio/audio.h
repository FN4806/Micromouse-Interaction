#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>

namespace audio {
        // Needs to be a softly typed enum for use with the pre-existing DFPlayer Library which expects an int value for song number
    enum song : uint8_t {
    TANK = 1,
    MENU_CLICK,
    DOOM,
    WIN_XP,
    WIN_95,
    NUM_SONGS
    };

    bool setup();

    void setVolume(uint8_t volume);

    void playMenuClick();
    void playStartupSound(uint8_t song);
    void playModeSound(uint8_t song);
}

#endif