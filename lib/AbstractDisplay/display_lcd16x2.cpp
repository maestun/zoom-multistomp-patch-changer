
#ifdef USE_1602_LCD

#include <LiquidCrystal_I2C.h>
// #include <LCDBigNumbers.hpp>

#include "display_lcd16x2.h"

LiquidCrystal_I2C _display(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
// LCDBigNumbers _biglcd(&_display, BIG_NUMBERS_FONT_3_COLUMN_2_ROWS_VARIANT_2);

LCD16x2Display::LCD16x2Display() {
    _display.init();
    _display.backlight();
}

void LCD16x2Display::clear() {
    _display.clear();
}

void LCD16x2Display::showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) {
    // clear();
    _display.setCursor(aX, aY);
    _display.print(aMessage);
    _display.display();
}

void LCD16x2Display::showString(const char * aMessage, uint16_t aX, uint16_t aY) {
    // clear();
    _display.setCursor(aX, aY);
    _display.print(aMessage);
    _display.display();
}

void LCD16x2Display::showPatch(uint8_t _currentPatch, char* _currentPatchName) {
    clear();
    // _biglcd.begin();
    // _biglcd.setBigNumberCursor(0);
    // uint8_t p = _currentPatch + 1;
    // if (p < 10) {
    //     _biglcd.print(0);
    //     _biglcd.writeAt(p, 3, 0);
    // }
    // else {
    //     _biglcd.print(p);
    // }


    char str[16] = {0};
    sprintf(str, "%02d - %s", _currentPatch + 1, _currentPatchName);
    _display.print(str);

    // _display.setCursor(6, 0);
    // _display.print(_currentPatchName);
    _display.display();
}

void LCD16x2Display::showRemoteInfo(const __FlashStringHelper * tag, const __FlashStringHelper * hash) {
    clear();
    showString(F("ZOOM MS REMOTE"), 0, 0);
    showString(hash, 0, 1);
    showString(tag, 11, 1);
}

void LCD16x2Display::showDeviceInfo(const __FlashStringHelper * name, const char * fw_version) {
    clear();
    showString(F("USB INIT OK"), 0, 0);
    showString(name, 0, 1);
    showString(fw_version, 12, 1);
}

#endif