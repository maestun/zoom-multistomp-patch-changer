#include <LiquidCrystal_I2C.h>

#include "display_lcd.h"

LiquidCrystal_I2C _display(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

LCDDisplay::LCDDisplay() {
    _display.init();
}

void LCDDisplay::clear() {
    _display.clear();
}

void LCDDisplay::showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) {
    clear();
    _display.setCursor(aX, aY);
    _display.println(aMessage);
    _display.display();
}

void LCDDisplay::showString(const char * aMessage, uint16_t aX, uint16_t aY) {
    clear();
    _display.setCursor(aX, aY);
    _display.println(aMessage);
    _display.display();
}

void LCDDisplay::showPatch(uint8_t _currentPatch, char* _currentPatchName) {
    uint8_t p = _currentPatch + 1;
    clear();
    _display.setCursor(0, 0);
    _display.println(_currentPatchName);
    _display.setCursor(0, 1);
    if (p < 10) {
        _display.print("0");  
    }
    _display.println(p);
    _display.display();
}