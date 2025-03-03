#pragma once

#include "display.h"

class LCDDisplay : public IDisplay {

public: 
    LCDDisplay();
    void clear() override;
    void showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) override;
    void showString(const char * aMessage, uint16_t aX, uint16_t aY) override;
    void showPatch(uint8_t _currentPatch, char* _currentPatchName) override;
};