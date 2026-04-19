#include <Arduino.h>
#include <U8g2lib.h>
#include <DFRobotDFPlayerMini.h>

#include "config/settings.h"
#include "config/bitmaps.h"
#include "ui_serial.h"

#define DFPSerial Serial1
//#define CommSerial Serial2

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
DFRobotDFPlayerMini df_player;
//Pi2PiSerial mcu_comms;

// ------ MAIN MENU ITEMS ARRAYS ------
const int MAIN_NUM_ITEMS = 5;
const unsigned char* const main_menu_icons[MAIN_NUM_ITEMS] = {
  image_Line_Icon_bits,
  image_Combat_Icon_bits,
  
  image_Music_Icon_bits,
  image_Target_bits,
  
  image_Avoid_Icon_bits
};
const char* const main_menu_items [MAIN_NUM_ITEMS] = {
  "Line Following",
  "Combat",

  "Music Options",
  "Calibration",

  "Obst. Avoidance"
};

// ------ SOUND MENU ITEMS ARRAYS ------
const int SOUND_NUM_ITEMS = 3;
const unsigned char* const sound_menu_icons[SOUND_NUM_ITEMS] = {
  image_Volume_Icon_bits,
  image_Hourglass_Icon_bits,
  image_Arrow_Left_bits
};
const char* const sound_menu_items[SOUND_NUM_ITEMS] = {
  "Volume",
  "Startup Sound",
  "Return"
};

// ------ STARTUP SOUND MENU ------
const int STARTUP_MENU_ITEMS = 3;
const unsigned char* const startup_menu_icons[STARTUP_MENU_ITEMS] = {
  image_Win_95_bits,
  image_Win_XP_bits,
  image_Arrow_Left_bits
};
const char* const startup_menu_items[STARTUP_MENU_ITEMS] = {
  "Windows 95",
  "Windows XP",
  "Return"
};

// ------ CALIBRATION MENU ------
const int CALIBRATION_NUM_ITEMS = 4;
const unsigned char* const calibration_menu_icons[CALIBRATION_NUM_ITEMS] = {
  image_Black_Circle_bits,
  image_White_Circle_bits,
  image_Calibration_Icon_bits,
  image_Arrow_Left_bits

};
const char* const calibration_menu_items[CALIBRATION_NUM_ITEMS] = {
  "Calibrate Black",
  "Calibrate White",
  "Read Thresholds",
  "Return"
};

// Needs to be a softly typed enum for use with the pre-existing DFPlayer Library which expects an int value for song number
enum DF_songs {
  TANK = 1,
  MENU_CLICK,
  DOOM,
  WIN_XP,
  WIN_95,
  NUM_SONGS
};

enum ErrorStates {
  DFPLAYER_INIT_FAIL = 0,
  SERIAL_MSG_FAIL
};

enum MenuState {
  MENU_MAIN = 0,
  MENU_SOUND,
  MENU_VOLUME,
  MENU_CALIBRATION,
  MENU_STARTUP,
  MENU_COUNT
};

struct MenuDef {
  const char* const* items;
  const unsigned char* const* icons;
  const uint8_t count;
};

const MenuDef menus[MENU_COUNT] = {
  {main_menu_items, main_menu_icons, MAIN_NUM_ITEMS},
  {sound_menu_items, sound_menu_icons, SOUND_NUM_ITEMS},
  {}, // Empty Menu State for Volume Since it uses a Different Menu Style
  {calibration_menu_items, calibration_menu_icons, CALIBRATION_NUM_ITEMS}, // Empty Menu State for Calibration Since it uses a Different Menu Style
  {startup_menu_items, startup_menu_icons, STARTUP_MENU_ITEMS}
};

MenuState current_menu = MENU_MAIN;
int current_option = 0;
int mode_engaged = 0;

const int enc_pin_A = 11;
const int enc_pin_B = 12;
const int enc_switch = 10;

volatile int previous = 0;
const int8_t decoder[16] {
   0, -1, +1,  0,
  +1,  0,  0, -1,
  -1,  0,  0, +1,
   0, +1, -1,  0
};

volatile int enc_delta = 0;
volatile int enc_accumulator = 0;

volatile int menu_btn_flag = 0;

bool ok = false;
// SERIAL HERE

void EncoderSwitch() {
  static int last_pressed = 0;
  if (millis() - last_pressed > 200) {
    menu_btn_flag = 1;
  }
  last_pressed = millis();
}

void EncoderISR() {
  uint8_t current = (digitalRead(enc_pin_A) << 1) | digitalRead(enc_pin_B);
  uint8_t index = (previous << 2) | current;
  int8_t step = decoder[index];

  if (step) {
    enc_accumulator += step;
    
    if (enc_accumulator >= 2) {enc_delta++; enc_accumulator = 0;}
    else if (enc_accumulator <= -2) {enc_delta--; enc_accumulator = 0;}
  }

  previous = current;
}



void UpdateMenu() {
  if (current_menu >= MENU_COUNT) current_menu = MENU_MAIN;
  const MenuDef& menu = menus[current_menu];  
  
  if (menu.count == 0) return;
  if (current_option >= menu.count) current_option = 0;

  uint8_t top_option = (current_option == 0) ? (menu.count - 1) : (current_option - 1);
  uint8_t middle_option = current_option;
  uint8_t bottom_option = (current_option + 1) % menu.count;

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.drawXBM(125, 1, 1, 61, image_Scroll_Bar_bits);
  u8g2.drawXBM(1, 22, 122, 21, image_Selected_Option_bits);

  u8g2.drawXBM(3, 3, 16, 16, menu.icons[top_option]);
  u8g2.drawStr(25, 16, menu.items[top_option]);

  u8g2.drawXBM(3, 24, 16, 16, menu.icons[middle_option]);
  u8g2.drawStr(25, 38, menu.items[middle_option]);

  u8g2.drawXBM(3, 47, 16, 16, menu.icons[bottom_option]);
  u8g2.drawStr(25, 60, menu.items[bottom_option]);

  int spacing = 64/menu.count;
  int bar_spacing = current_option * spacing;
  u8g2.drawBox(124, bar_spacing, 3, spacing);

  u8g2.sendBuffer();

  df_player.playMp3Folder(DF_songs::MENU_CLICK);
}

void ErrorScreen(ErrorStates error_type) {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  switch (error_type) {
    case ErrorStates::DFPLAYER_INIT_FAIL:
      u8g2.setFont(u8g2_font_profont15_tr);
      u8g2.drawStr(22, 14, "ERROR CODE 1");

      u8g2.setFont(u8g2_font_profont11_tr);
      u8g2.drawStr(1,29,"Unable to connect to:");
      u8g2.drawStr(22,37, "DF Player Mini");
      u8g2.drawStr(2, 50, "1. Check Connection");
      u8g2.drawStr(2, 61, "2. Insert SD Card");

      u8g2.drawXBM(110,2,16,16,image_Warning_bits);
      u8g2.drawXBM(2,2, 16, 16, image_Warning_bits);

      break;
    case ErrorStates::SERIAL_MSG_FAIL:
      u8g2.setFont(u8g2_font_profont15_tr);
      u8g2.drawStr(22, 14, "ERROR CODE 2");

      u8g2.setFont(u8g2_font_profont11_tr);
      u8g2.drawStr(1,29,"No Response from Main Pico");
      u8g2.drawStr(1,37,"1. Check Connections");
      u8g2.drawStr(1,45,"2. Check Main Pico is On");

      u8g2.drawXBM(110,2,16,16,image_Warning_bits);
      u8g2.drawXBM(2,2, 16, 16, image_Warning_bits);

      break;
  }

  u8g2.sendBuffer();
}

void DisplayFace() {
  df_player.playMp3Folder(DF_songs::MENU_CLICK);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_6x13_tr);

  switch (current_option) {
    case 0: // Line Following Mode
      
      ok = sendSetModeCommand(MODE_LINE_FOLLOWING);
      if (!ok) {
        ErrorScreen(ErrorStates::SERIAL_MSG_FAIL);
        mode_engaged = 1;
      }

      break;
    case 1: // Combat Mode
      u8g2.drawXBM(14, 8, 101, 48, image_Angry_Face_bits);
      
      ok = sendSetModeCommand(MODE_COMBAT);
      if (!ok) {
        ErrorScreen(ErrorStates::SERIAL_MSG_FAIL);
        mode_engaged = 1;
      }
      break;
    case (MAIN_NUM_ITEMS - 1): // Obstacle Avoidance Mode
      
      ok = sendSetModeCommand(MODE_OBSTACLE_AVOIDANCE);
      if (!ok) {
        ErrorScreen(ErrorStates::SERIAL_MSG_FAIL);
        mode_engaged = 1;
      }
      break;
    default:
      u8g2.drawStr(10,10,{"No Face Yet!"});
      break;
  }

  u8g2.sendBuffer();

  switch (current_option) {
    case 0:
      df_player.playMp3Folder(DF_songs::TANK);
      break;
    case 1:
      df_player.playMp3Folder(DF_songs::DOOM);
      break;
    default:
      break;
  }
}

void VolumeMenu() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.drawStr(25, 14, "Adjust Volume");

  int volume_bar = 104 * settings::ui_volume / 30;

  u8g2.drawFrame(11, 24, 106, 16);
  u8g2.drawBox(12, 25, volume_bar, 14);

  u8g2.drawStr(11, 56, "-");
  u8g2.drawStr(111, 56, "+");

  u8g2.sendBuffer();
}

void CalibrationMenu() {
  int gif_index = 0;
  int jump_index = 0;
  while(true) {
    u8g2.clearBuffer();

    u8g2.drawXBM((-40 + (gif_index * 3) + (jump_index * 22)), 15, 40, 20, mouse_gif[gif_index]);
    if (gif_index >=6) {gif_index = 0; jump_index++;} else {gif_index++;}  

    if (jump_index >= 8) {break;}

    u8g2.sendBuffer();
    delay(100);
  }
}

void DisplayThresholds() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_profont10_tr);
  
  u8g2.drawXBM(2, 2, 16, 16, image_Wheel_bits);
  u8g2.drawXBM(2, 45, 16, 16, image_Wheel_bits);
  u8g2.drawXBM(110, 45, 16, 16, image_Wheel_bits);
  u8g2.drawXBM(110, 2, 16, 16, image_Wheel_bits);
  
  u8g2.drawStr(25, 9, "FL");
  u8g2.drawStr(25, 52, "RL");
  u8g2.drawStr(94, 52, "RR");
  u8g2.drawStr(94, 9, "FR");
  
  u8g2.drawStr(4, 35, "- CALIBRATION COMPLETE -");
  
  u8g2.drawStr(25, 60, "2.6V");
  u8g2.drawStr(84, 60, "2.6V");
  u8g2.drawStr(25, 17, "2.6V");
  u8g2.drawStr(84, 17, "2.6V");

  u8g2.sendBuffer();
}

void LoadingScreen() {
  int gif_index = 0;
  int jump_index = 0;

  while(true) {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(u8g2_font_profont29_tr);
    u8g2.drawStr(31, 23, "TEAM");
    u8g2.drawStr(24, 60, "BRAVO");

    u8g2.drawXBM((-40 + (gif_index * 3) + (jump_index * 22)), 25, 40, 20, mouse_gif[gif_index]);
    if (gif_index >=6) {gif_index = 0; jump_index++;} else {gif_index++;}  

    if (jump_index >= 8) {break;}

    u8g2.sendBuffer();
    delay(100);
  }
}


void NextOption() {
  const uint8_t num_options = menus[current_menu].count;
  current_option = (current_option + 1) % num_options;
  UpdateMenu();
}

void PrevOption() {
  const uint8_t num_options = menus[current_menu].count;
  current_option = (current_option == 0) ? (num_options - 1) : (current_option - 1);
  UpdateMenu();
}


void SelectOption() {
  switch (current_menu) {
    case MENU_MAIN:
      if (mode_engaged == 0 && current_option == 2) {
        // Sound menu has been selected
        current_menu = MENU_SOUND;
        UpdateMenu();
      } else if (mode_engaged == 0 && current_option == 3) {
        // Calibration Menu
        current_menu = MENU_CALIBRATION;
        UpdateMenu();
      } else if (mode_engaged == 0) {
        // Mode is not engaged and option is NOT for sound menu
        DisplayFace();
        mode_engaged = 1;
      } else {
        UpdateMenu();
        
        ok = sendSetModeCommand(MODE_UI);
        if (!ok) {
          ErrorScreen(ErrorStates::SERIAL_MSG_FAIL);
          mode_engaged = 1;
          break;
        }

        mode_engaged = 0;
      }
      break;

    case MENU_SOUND:
      if (current_option == 0) {
        // Volume menu
        current_menu = MENU_VOLUME;
        VolumeMenu();
      } else if (current_option == 1) {
        // Startup menu
        current_menu = MENU_STARTUP;
        UpdateMenu();
      } else {
        // Return
        current_option = MENU_SOUND;
        current_menu = MENU_MAIN;
        UpdateMenu();
      }
      break;

    case MENU_VOLUME:
      current_menu = MENU_SOUND;
      //mcu_comms.SetDefaultVolume(settings::ui_volume);
      UpdateMenu();
      break;

    case MENU_STARTUP:
      if (current_option == 0) {
        // Windows 95
        df_player.playMp3Folder(DF_songs::WIN_95);
      } else if (current_option == 1) {
        // Windows XP
        df_player.playMp3Folder(DF_songs::WIN_XP);
      } else {
        // Return to previous menu
        current_option = MENU_STARTUP;
        current_menu = MENU_SOUND;
        UpdateMenu();
      }
      break;

    case MENU_CALIBRATION:
    
      if (mode_engaged == 0 & current_option == 0) {
        // Calibrate LDRs on Black Surface
        CalibrationMenu();
        mode_engaged = 1;
      } else if (mode_engaged == 0 & current_option == 1) {
        // Calibrate LDRs on White Line

        mode_engaged = 1;
      } else if (mode_engaged == 0 & current_option == 2) {
        // Read Thresholds
        DisplayThresholds();
        mode_engaged = 1;
      } else if (mode_engaged == 0 & current_option == 3) {
        // Return to previous menu
        current_option = MENU_CALIBRATION;
        current_menu = MENU_MAIN;
        mode_engaged = 0;
        UpdateMenu();
      } else {
        mode_engaged = 0;
        UpdateMenu();
      }
      break;

    default:
      break;
  }
}

void set_volume(int volume) {
  settings::ui_volume = constrain(volume, 0, 30);
  df_player.volume(settings::ui_volume);
}

void setup() {
  // Debugging Serial Communications using built-in Serial over USB UART
  Serial.begin(9600);

  // MCU to MCU Communications using built-in Serial2 on hardware UART 1
  // Mapped to GP8 and GP9
  // CommSerial.setTX(8);
  // CommSerial.setRX(9);
  // CommSerial.begin(115200);

  setupUiLink();

  // DFPlayer Mini Communication using buult-in Serial1 on hardware UART 0
  // Mapped to GP0 and GP1 by default.
  DFPSerial.setTX(settings::df_tx);
  DFPSerial.setRX(settings::df_rx);
  DFPSerial.begin(settings::df_baud);

  // Initialise DFPlayer Mini serial communication with a catch if initialisation fails
  if (!df_player.begin(DFPSerial, /*isACK = */true, /*doReset = */true)) {
    ErrorScreen(ErrorStates::DFPLAYER_INIT_FAIL);
    while(true){
      delay(0);
    }
  }

  // Initialise u8g2 OLED display library
  u8g2.begin();

  
  pinMode(enc_pin_A, INPUT_PULLUP);
  pinMode(enc_pin_B, INPUT_PULLUP);
  pinMode(enc_switch, INPUT_PULLUP);

  previous = (digitalRead(enc_pin_A) << 1) | digitalRead(enc_pin_B);

  // Atach a CHANGE interrupt to enc_pin_A and exectute the update function when this change occurs.
  attachInterrupt(digitalPinToInterrupt(enc_pin_A), EncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_pin_B), EncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc_switch), EncoderSwitch, FALLING);

  settings::startup_sound = DF_songs::WIN_95;
  settings::combat_music = DF_songs::DOOM;

  // Fetch the settings from EEPROM, set the ACK flag and wait for it to be cleared by the serial handler, on error, resort to defaults

  //df_player.volume(ui_volume);
  df_player.playMp3Folder(settings::startup_sound);

  LoadingScreen();
  UpdateMenu();
}

void loop() {

  // Read encoder steps while interrupts are disabled, to avoid a state change mid-read
  int steps;
  noInterrupts();
  steps = enc_delta;
  enc_delta = 0;
  interrupts();

  // Menu handling switch case
  switch (current_menu) {

    case (MENU_VOLUME):
      while (steps > 0) {set_volume(settings::ui_volume + 1); steps--;}
      while (steps < 0) {set_volume(settings::ui_volume - 1); steps++;}
      steps =  0;
      VolumeMenu();
      break;

    default:
      if (mode_engaged != 0) break;

      while (steps > 0) {NextOption(); steps--;}
      while (steps < 0) {PrevOption(); steps++;}
      steps = 0;
      break;
  }

  // If the encoder's push button has been pressed, handle using the SelectOption function
  if (menu_btn_flag == 1) {
    SelectOption();
    menu_btn_flag = 0;
  }

  // char line[96];
  // while(mcu_comms.PollLine(line, sizeof(line))) {
  //   mcu_comms.HandleMainLine(line);
  // }

  if (flags::volume_change) {
    df_player.volume(settings::ui_volume);
    flags::volume_change = false;
  }

}