#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <Arduino.h>

class IDisplay {
public:
    virtual ~IDisplay() = default; // No need for "override" here!
    virtual void clear() = 0;
    virtual void showString(const __FlashStringHelper* aMessage, uint16_t aX, uint16_t aY) = 0;
    virtual void showString(const char* aMessage, uint16_t aX, uint16_t aY) = 0;
    virtual void showPatch(uint8_t _currentPatch, char* _currentPatchName) = 0;

    virtual void showRemoteInfo(const __FlashStringHelper * tag, const __FlashStringHelper * hash) = 0;
    virtual void showDeviceInfo(const __FlashStringHelper * name, const char * fw_version) = 0;
};

#endif
