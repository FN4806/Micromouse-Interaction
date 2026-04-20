#include <Arduino.h>
#include <u8g2lib.h>

#include "ui/display.h"
#include "config/bitmaps.h"
#include "ui_serial.h"

namespace display {

    namespace {
        U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
    }

    void setup() {
        // Initialize the display here if needed
        u8g2.begin();
    }

    void showMenu(MenuState current_menu, int current_option) {
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

        u8g2.drawXBM(3, 3, 16, 16, menu.items[top_option].icon);
        u8g2.drawStr(25, 16, menu.items[top_option].label);

        u8g2.drawXBM(3, 24, 16, 16, menu.items[middle_option].icon);
        u8g2.drawStr(25, 38, menu.items[middle_option].label);

        u8g2.drawXBM(3, 47, 16, 16, menu.items[bottom_option].icon);
        u8g2.drawStr(25, 60, menu.items[bottom_option].label);

        int spacing = 64/menu.count;
        int bar_spacing = current_option * spacing;
        u8g2.drawBox(124, bar_spacing, 3, spacing);

        u8g2.sendBuffer();

        //df_player.playMp3Folder(DF_songs::MENU_CLICK);
    }

    void showError(ErrorStates error_type) {
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

    void showVolume(uint8_t volume) {
        u8g2.clearBuffer();
        u8g2.setFontMode(1);
        u8g2.setBitmapMode(1);
        u8g2.setFont(u8g2_font_6x13_tr);
        u8g2.drawStr(25, 14, "Adjust Volume");

        int volume_bar = 104 * ui_settings.volume / 30;

        u8g2.drawFrame(11, 24, 106, 16);
        u8g2.drawBox(12, 25, volume_bar, 14);

        u8g2.drawStr(11, 56, "-");
        u8g2.drawStr(111, 56, "+");

        u8g2.sendBuffer();
    }

    void showThresholds() {
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

    void showModeFace(int current_option) {
        u8g2.clearBuffer();
        u8g2.setFontMode(1);
        u8g2.setBitmapMode(1);
        u8g2.setFont(u8g2_font_6x13_tr);

        switch (current_option) {
            case MAIN_MENU_LINE: // Line Following Mode
            break;
            case MAIN_MENU_COMBAT: // Combat Mode
            u8g2.drawXBM(14, 8, 101, 48, image_Angry_Face_bits);
            break;
            case MAIN_MENU_OBSTACLE_AVOIDANCE: // Obstacle Avoidance Mode
            break;
            default:
            u8g2.drawStr(10,10,{"No Face Yet!"});
            break;
        }

        u8g2.sendBuffer();
    }

    void showLoading() {
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
};