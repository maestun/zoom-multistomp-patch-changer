#pragma once

#include "display.h"

class LCD16x2Display : public IDisplay {

public: 
    LCD16x2Display();
    void clear() override;
    void showString(const __FlashStringHelper * aMessage, uint16_t aX, uint16_t aY) override;
    void showString(const char * aMessage, uint16_t aX, uint16_t aY) override;
    void showPatch(uint8_t _currentPatch, char* _currentPatchName) override;
    void showRemoteInfo(const __FlashStringHelper * tag, const __FlashStringHelper * hash);
    void showDeviceInfo(const __FlashStringHelper * name, const char * fw_version);

};