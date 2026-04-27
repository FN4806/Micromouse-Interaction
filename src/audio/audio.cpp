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
        if (!df_player.begin(DFPSerial, /*isACK = */false, /*doReset = */true)) {
            return false;
        }
        // Note:    The DFPlayer library will endlessly wait for an ACK signal if required, this results in blocking errors and effective crashes. So for now isACK is set to false, meaning no ACK signal is waited for.
        //          This does mean that we can never be sure commands are recieved or if the module is initialised correctly but since it's only audio, it's not a critical system and this can be accepted.

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