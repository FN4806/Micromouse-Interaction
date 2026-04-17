#ifndef DISPLAY_H
#define DISPLAY_H

enum ErrorStates {
  DFPLAYER_INIT_FAIL = 0
};

class Display {
    public:
        Display();
        void UpdateMenu();
        void DisplayFace();
        void VolumeMenu();
        void CalibrationMenu();
        void DisplayThresholds();
        void LoadingScreen();
        void ErrorScreen(ErrorStates error_type);

    private:
        
};

#endif