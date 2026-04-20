#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

#include "audio/audio.h"
#include "config/settings.h"

namespace audio {
    
    namespace {
        #define DFPSerial Serial1

        DFRobotDFPlayerMini df_player;
    }

    bool setup() {
        // DFPlayer Mini Communication using buult-in Serial1 on hardware UART 0
        // Mapped to GP0 and GP1 by default.
        DFPSerial.setTX(settings::df_tx);
        DFPSerial.setRX(settings::df_rx);
        DFPSerial.begin(settings::df_baud);

        // Initialise DFPlayer Mini serial communication with a catch if initialisation fails
        if (!df_player.begin(DFPSerial, /*isACK = */true, /*doReset = */true)) {
            return false;
        }
        return true;
    }

    void setVolume(uint8_t volume) {
        volume = constrain(volume, 0, 30);
        df_player.volume(volume);
    }

    void playMenuClick() {
        df_player.playMp3Folder(song::MENU_CLICK);
    }

    void playStartupSound(uint8_t song) {
        df_player.playMp3Folder(song);
    }

    void playModeSound(uint8_t song) {
        df_player.playMp3Folder(song);
    }
}