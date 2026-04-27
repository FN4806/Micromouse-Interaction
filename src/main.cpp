#include <Arduino.h>

#include "input/encoder.h"
#include "config/settings.h"
#include "config/bitmaps.h"
#include "ui_serial.h"
#include "ui/display.h"
#include "ui/menu_data.h"
#include "audio/audio.h"

MenuState current_menu = MENU_MAIN;
int current_option = 0;

void enterMode() {
  audio::playMenuClick();
  
  display::showModeFace(current_option);

  switch (current_option) {
    case MainMenuItems::MAIN_MENU_LINE:
      audio::playModeSound(audio::TANK);
      break;
    case MainMenuItems::MAIN_MENU_COMBAT:
      audio::playModeSound(audio::DOOM);
      break;
    default:
      break;
  }
}


void nextOption() {
  const uint8_t num_options = menus[current_menu].count;

  if (num_options == 0) return; // Gaurd against div by 0 errors
  current_option = (current_option + 1) % num_options;
  
  audio::playMenuClick();
  display::showMenu(current_menu, current_option);
}

void prevOption() {
  const uint8_t num_options = menus[current_menu].count;

  if (num_options == 0) return; // Gaurd against div by 0 errors
  current_option = (current_option == 0) ? (num_options - 1) : (current_option - 1);
  
  audio::playMenuClick();
  display::showMenu(current_menu, current_option);
}


void selectOption() {
  const MenuDef& menu = menus[current_menu];

  if (menu.count == 0 || menu.items == nullptr) {
    return;
  }

  MenuAction action = menu.items[current_option].action;

  switch (action) {
    case ACTION_MODE_LINE:
      ui_state = UI_MODE_RUNNING;
      // Call Line Following UI
      // Call Line Following Sound
      enterMode(); // NEEDS TO BE CHANGED TO SPLIT UP FUNCTIONALITY AND REDUCE EXTRA CONDITIONALS
      // Send Line Following Command to Main Pico
      if (!sendSetModeCommand(MODE_LINE_FOLLOWING)) {
        display::showError(SERIAL_MSG_FAIL);
        ui_state = UI_SHOWING_ERROR;
      }
      break;

    case ACTION_MODE_COMBAT:
      ui_state = UI_MODE_RUNNING;  
      // Call Combat UI
      // Call Combat Sound
      enterMode(); // NEEDS TO BE CHANGED TO SPLIT UP FUNCTIONALITY AND REDUCE EXTRA CONDITIONALS
      // Send Combat Command to Main Pico
      if (!sendSetModeCommand(MODE_COMBAT)) {
        display::showError(SERIAL_MSG_FAIL);
        ui_state = UI_SHOWING_ERROR;
      }
      break;

    case ACTION_MODE_OBSTACLE:
      ui_state = UI_MODE_RUNNING;
      // Call Obstacle Avoidance UI
      // Call Obstacle Avoidance Sound
      enterMode(); // NEEDS TO BE CHANGED TO SPLIT UP FUNCTIONALITY AND REDUCE EXTRA CONDITIONALS
      // Send Obstacle Avoidance Command to Main Pico
      if (!sendSetModeCommand(MODE_OBSTACLE_AVOIDANCE)) {
        display::showError(SERIAL_MSG_FAIL);
        ui_state = UI_SHOWING_ERROR;
      }
      break;
    

    case ACTION_ENTER_SOUND_MENU:
      current_menu = MENU_SOUND;
      display::showMenu(current_menu, current_option);
      break;

    case ACTION_ENTER_STARTUP_MENU:
      current_menu = MENU_STARTUP;
      display::showMenu(current_menu, current_option);
      break;

    case ACTION_ENTER_CALIBRATION_MENU:
      current_menu = MENU_CALIBRATION;
      display::showMenu(current_menu, current_option);
      break;

    case ACTION_ENTER_COMBAT_MENU:
      current_menu = MENU_COMBAT;
      display::showMenu(current_menu, current_option);
      break;  

    case ACTION_VOLUME_SCREEN:
      ui_state = UI_VOLUME_EDIT;
      current_menu = MENU_VOLUME;
      display::showVolume(ui_settings.volume);
      break;


    case ACTION_STARTUP_SOUND_WIN95:
      ui_settings.startup_music = audio::WIN_95; // Store Selection in RAM
      audio::playStartupSound(ui_settings.startup_music); // Play Sound
      break;

    case ACTION_STARTUP_SOUND_WINXP:
      ui_settings.startup_music = audio::WIN_XP; // Store Selection in RAM
      audio::playStartupSound(ui_settings.startup_music); // Play Sound
      break;

    case ACTION_COMBAT_SOUND_DOOM:
      ui_settings.combat_music = audio::DOOM; // Store Selection in RAM
      audio::playStartupSound(ui_settings.combat_music); // Play Sound
      break;  

    case ACTION_COMBAT_SOUND_DUEL_FATES:
      ui_settings.combat_music = audio::DUEL_FATES; // Store Selection in RAM
      audio::playStartupSound(ui_settings.combat_music); // Play Sound
      break;

    case ACTION_CALIBRATE_BLACK:
      //Display loading screen before requesting data
      calibrateBlack();
      display::showThresholds(THRESH_BLACK);
      ui_state = UI_SHOWING_THRESHOLDS;
      break;

    case ACTION_CALIBRATE_WHITE:
      //Display loading screen before requesting data
      calibrateWhite();
      display::showThresholds(THRESH_WHITE);
      ui_state = UI_SHOWING_THRESHOLDS;
      break;

    case ACTION_READ_THRESHOLDS:
      loadThresholds(); // Read the thresholds from the main pico
      display::showThresholds(THRESH_DISPLAY); // Display thresholds
      ui_state = UI_SHOWING_THRESHOLDS;
      break;

    case ACTION_RETURN:
      switch (current_menu) {
        case MENU_VOLUME:
          saveSetting(SETTING_VOLUME, ui_settings.volume); // Store RAM volume value in EEPROM
          current_menu = MENU_SOUND;
          break;
        case MENU_SOUND:
          current_menu = MENU_MAIN;
          break;
        case MENU_STARTUP:
          saveSetting(SETTING_STARTUP_MUSIC, ui_settings.startup_music); // Store RAM startup sound value in EEPROM
          current_menu = MENU_SOUND;
          break;
        case MENU_CALIBRATION:
          current_menu = MENU_MAIN;
          break;
        case MENU_COMBAT:
          saveSetting(SETTING_COMBAT_MUSIC, ui_settings.combat_music); // Store RAM combat sound value in EEPROM
          current_menu = MENU_SOUND;
          break;
        default:
          current_menu = MENU_MAIN;
          break;
      }
      current_option = 0;
      display::showMenu(current_menu, current_option);
      break;

    default:
      break;
  }
}

void handleButtonPress() {
  switch (ui_state) {
    case UI_NAVIGATING:
      selectOption();
      break;

    case UI_MODE_RUNNING:
      ui_state = UI_NAVIGATING;  
      if (!sendSetModeCommand(MODE_UI)) {
        display::showError(SERIAL_MSG_FAIL);
        ui_state = UI_SHOWING_ERROR;
      }
      display::showMenu(current_menu, current_option);
      break;

    case UI_VOLUME_EDIT:
      ui_state = UI_NAVIGATING;
      current_menu = MENU_SOUND;
      saveSetting(SETTING_VOLUME, ui_settings.volume);
      display::showMenu(current_menu, current_option);
      break;

    case UI_SHOWING_THRESHOLDS:
      ui_state = UI_NAVIGATING;
      display::showMenu(current_menu, current_option);
      break;

    case UI_SHOWING_ERROR:
      ui_state = UI_NAVIGATING;
      display::showMenu(current_menu, current_option);
      break;
  }
}

void setup() {
  // Debugging Serial Communications using built-in Serial over USB UART
  Serial.begin(9600);

  setupUiLink();

  if (!audio::setup()) {
    display::showError(DFPLAYER_INIT_FAIL);
    while (true) {delay(0);}
  }

  // Initialise u8g2 OLED display library
  display::setup();

  encoder::initEncoder();

  ui_settings.startup_music = audio::WIN_95;
  ui_settings.combat_music = audio::DOOM;

  // Fetch the settings from EEPROM, set the ACK flag and wait for it to be cleared by the serial handler, on error, resort to defaults
  loadDefaultSettings();
  loadAllSettings();

  audio::setVolume(ui_settings.volume);
  audio::playStartupSound(ui_settings.startup_music);

  display::showLoading();
  display::showMenu(current_menu, current_option);
}

void loop() {

  // Read encoder steps
  int steps = encoder::getEncoderDelta();

  // Menu handling switch case
  switch (current_menu) {

    case (MENU_VOLUME):
      while (steps > 0) {(ui_settings.volume < 30) ? ui_settings.volume++ : ui_settings.volume = 30; steps--;}
      while (steps < 0) {(ui_settings.volume > 0) ? ui_settings.volume-- : ui_settings.volume = 0; steps++;}
      steps =  0;
      audio::setVolume(ui_settings.volume);
      display::showVolume(ui_settings.volume);
      break;

    default:
      if (ui_state != UI_NAVIGATING) break;

      while (steps > 0) {nextOption(); steps--;}
      while (steps < 0) {prevOption(); steps++;}
      steps = 0;
      break;
  }

  // If the encoder's push button has been pressed, handle using the handleButtonPress function
  if (flags::menu_btn_flag == true) {
    audio::playMenuClick();
    handleButtonPress();
    flags::menu_btn_flag = false;
  }

  // char line[96];
  // while(mcu_comms.PollLine(line, sizeof(line))) {
  //   mcu_comms.HandleMainLine(line);
  // }

  if (flags::volume_change) {
    audio::setVolume(ui_settings.volume);
    flags::volume_change = false;
  }

}