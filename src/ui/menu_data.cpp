#include <Arduino.h>

#include "ui/menu_data.h"
#include "config/bitmaps.h"

static const MenuItem main_menu_items[] = {
    {"Line Following", image_Line_Icon_bits, ACTION_MODE_LINE},
    {"Combat", image_Combat_Icon_bits, ACTION_MODE_COMBAT},
    {"Music Options", image_Music_Icon_bits, ACTION_ENTER_SOUND_MENU},
    {"Calibration", image_Target_bits, ACTION_ENTER_CALIBRATION_MENU},
    {"Obst. Avoidance", image_Avoid_Icon_bits, ACTION_MODE_OBSTACLE}
};

static const MenuItem sound_menu_items[] = {
    {"Volume", image_Volume_Icon_bits, ACTION_VOLUME_SCREEN},
    {"Startup Sound", image_Hourglass_Icon_bits, ACTION_ENTER_STARTUP_MENU},
    {"Combat Sound", image_Combat_Icon_bits, ACTION_ENTER_COMBAT_MENU},
    {"Return", image_Arrow_Left_bits, ACTION_RETURN}
};

static const MenuItem startup_menu_items[] = {
    {"Windows 95", image_Win_95_bits, ACTION_STARTUP_SOUND_WIN95},
    {"Windows XP", image_Win_XP_bits, ACTION_STARTUP_SOUND_WINXP},
    {"Return", image_Arrow_Left_bits, ACTION_RETURN}
};

static const MenuItem calibration_menu_items[] = {
    {"Calibrate Black", image_Black_Circle_bits, ACTION_CALIBRATE_BLACK},
    {"Calibrate White", image_White_Circle_bits, ACTION_CALIBRATE_WHITE},
    {"Read Thresholds", image_Calibration_Icon_bits, ACTION_READ_THRESHOLDS},
    {"Return", image_Arrow_Left_bits, ACTION_RETURN}
};

static const MenuItem combat_menu_items[] = {
    {"Doom", image_Combat_Icon_bits, ACTION_COMBAT_SOUND_DOOM},
    {"Duel of Fates", image_Combat_Icon_bits, ACTION_COMBAT_SOUND_DUEL_FATES},
    {"Return", image_Arrow_Left_bits, ACTION_RETURN}
};

const MenuDef menus[MENU_COUNT] = {
    {main_menu_items, 5},
    {sound_menu_items, 3},
    {nullptr, 0}, // Special Volume Screen
    {calibration_menu_items, 4},
    {startup_menu_items, 3},
    {combat_menu_items, 3}
};

UiInteractionState ui_state = UI_NAVIGATING;