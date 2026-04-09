#include "Arduino.h"
#include "pi2pi_serial.h"
#include "settings.h"

#define CommSerial Serial2

char Pi2PiSerial::lin_buff[96] = {};
size_t Pi2PiSerial::lin_len = 0;

Pi2PiSerial::Pi2PiSerial() {
    lin_len = 0;
}

void Pi2PiSerial::SendLine(const char* cmd) {
  CommSerial.print(cmd);
  CommSerial.print('\n');
}

bool Pi2PiSerial::PollLine(char* out, size_t out_size) {
  while (CommSerial.available() > 0) {
    char c = CommSerial.read();
    if (lin_len >= sizeof(lin_buff) - 1) {
      lin_len = 0;
      continue;
    } 
    if (c == '\n') {
      lin_buff[lin_len] = '\0';
      strncpy(out, lin_buff, out_size);
      out[out_size - 1] = '\0';
      lin_len = 0;
      return true;
    }
    lin_buff[lin_len++] = c;
  }
  return false;
}

int Pi2PiSerial::SplitCSV(char* str, char* out_tokens[], int max_tokens) {
  int count = 0;
  char* p = str;
  while (count < max_tokens) {
    out_tokens[count++] = p;
    char* comma = strchr(p, ',');
    if (!comma) break;
    *comma = '\0';
    p = comma + 1;
  }
  return count;
}

void Pi2PiSerial::RequestSettings() {
  // Send commands to retrieve each of the stored settings from the EEPROM controlled by main pico
  SendLine("GET,startup");
  SendLine("GET,combat_music");
  SendLine("GET,volume");
}

void Pi2PiSerial::HandleMainLine(char* line) {
  size_t n = strlen(line);
  if (n && line[n-1] == '\r') line[n-1] = '\0';

  char* tokens[4] = {0};
  int num_toks = SplitCSV(line, tokens, 4);
  if (num_toks <= 0) return;

  if (strcmp(tokens[0], "VAL") == 0 && num_toks >= 3) {
    const char* key = tokens[1];
    int val = atoi(tokens[2]);

    if (strcmp(key, "startup") == 0) settings::startup_sound = val;
    else if (strcmp(key, "combat_music") == 0) settings::combat_music = val;
    else if (strcmp(key, "volume") == 0) {settings::ui_volume = val; flags::volume_change = true;};

    return;
  }

  if (strcmp(tokens[0], "ACK") == 0) {
    // Command was recieved by main pico, could clear a flag or change a ui icon, undecided if I will use this yet
  }

  if (strcmp(tokens[0], "ERR") == 0) {
    // Error recieved from main pico, print to serial for debug, could set a flag or update a ui icon here too
    Serial.print("MAIN PICO ERROR: ");
    Serial.println(line);
  }
}

void Pi2PiSerial::SetDefaultVolume(uint8_t volume) {
  volume = constrain(volume, 0, 30);
  char cmd[48];
  sprintf(cmd, "SET,volume,%d", volume);
  SendLine(cmd);
}

void Pi2PiSerial::SetCombatMusic(uint8_t song, uint8_t NUM_SONGS) {
  song = constrain(song, 1, NUM_SONGS - 1);
  char cmd[48];
  sprintf(cmd, "SET,combat_music,%d", song);
  SendLine(cmd);
}

void Pi2PiSerial::SelectMode(uint8_t mode) {
  char cmd[48];
  sprintf(cmd, "SET,mode,%d", mode);
  SendLine(cmd);
}

// --------------------------------------------