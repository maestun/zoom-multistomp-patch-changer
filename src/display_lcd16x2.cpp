#include <LiquidCrystal_I2C.h>

#include "display_lcd16x2.h"
#include "version.h"

LiquidCrystal_I2C _display(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

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