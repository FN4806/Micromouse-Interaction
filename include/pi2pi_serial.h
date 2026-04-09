#ifndef PI2PI_SERIAL_H
#define PI2PI_SERIAL_H

#include "Arduino.h"

class Pi2PiSerial {
    
    public:
    Pi2PiSerial();
    void RequestSettings();
    void SetDefaultVolume(uint8_t volume);
    void SetCombatMusic(uint8_t song, uint8_t NUM_SONGS);
    void SetStartupMusic(uint8_t song, uint8_t NUM_SONGS);
    void SelectMode(uint8_t mode);
    bool PollLine(char* out, size_t out_size);
    void HandleMainLine(char* line);

    private:
    void SendLine(const char* cmd);
    int SplitCSV(char* str, char* out_tokens[], int max_tokens);
    static char lin_buff[96];
    static size_t lin_len;
};

#endif