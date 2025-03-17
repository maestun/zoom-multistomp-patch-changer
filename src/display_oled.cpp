#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "display_oled.h"


#define PIN_OLED_RESET              (-1)
#define SCREEN_WIDTH                (128)
#define SCREEN_HEIGHT               (32)


// display stuff
Adafruit_SSD1306 	_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_OLED_RESET);


OLEDDisplay::OLEDDisplay() {
    if(!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        // TODO: show error
    }
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);   
}

void OLEDDisplay::clear() {
    _display.clearDisplay();
}

void OLEDDisplay::showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) {
    clear();
    _display.setCursor(aX, aY);
    _display.println(aMessage);
    _display.display();
}

void OLEDDisplay::showString(const char * aMessage, uint16_t aX, uint16_t aY) {
    clear();
    _display.setCursor(aX, aY);
    _display.println(aMessage);
    _display.display();
}

void OLEDDisplay::showPatch(uint8_t _currentPatch, char* _currentPatchName) {
    uint8_t p = _currentPatch + 1;
    clear();
    _display.setCursor(0, 0);
    _display.println(_currentPatchName);
    _display.setCursor(100, 16);
    if (p < 10) {
        _display.print("0");  
    }
    _display.println(p);
    _display.display();
}