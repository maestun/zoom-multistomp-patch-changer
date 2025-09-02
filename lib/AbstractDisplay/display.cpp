#include "display_lcd16x2.h"

IDisplay* display_instance() {
    static IDisplay *  _disp = nullptr;
    if (_disp ==nullptr) {

    #ifdef USE_12832_OLED
        _disp = new OLEDDisplay();
    #elif defined(USE_1602_LCD)
        _disp = new LCD16x2Display();
    #endif
    }
    return _disp;
}