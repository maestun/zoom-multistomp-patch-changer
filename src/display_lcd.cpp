#include <LiquidCrystal_I2C.h>

#include "display_lcd.h"

LiquidCrystal_I2C _display(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

LCDDisplay::LCDDisplay() {
    _display.init();
    _display.backlight();
}

void LCDDisplay::clear() {
    _display.clear();
}

void LCDDisplay::showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) {
    // clear();
    _display.setCursor(aX, aY);
    _display.print(aMessage);
    _display.display();
}

void LCDDisplay::showString(const char * aMessage, uint16_t aX, uint16_t aY) {
    // clear();
    _display.setCursor(aX, aY);
    _display.print(aMessage);
    _display.display();
}

void LCDDisplay::showPatch(uint8_t _currentPatch, char* _currentPatchName) {
    clear();
    uint8_t p = _currentPatch + 1;
    _display.setCursor(0, 0);
    _display.print(_currentPatchName);
    _display.setCursor(0, 1);
    if (p < 10) {
        _display.print("0");  
    }
    _display.print(p);
    _display.display();
}

void LCDDisplay::showRemoteInfo(uint8_t _currentPatch, char* _currentPatchName) {
    clear();
    showString(F(" ZOOM MS REMOTE "), 0, 0);
    showString(GIT_TAG, 0, 1); // oled y=16 / TODO: get_line2() for IDisplay
    showString(GIT_HASH, 7, 1);// oled y=16 / TODO: get_line2() for IDisplay
}