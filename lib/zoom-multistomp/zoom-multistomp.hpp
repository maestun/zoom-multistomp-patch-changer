#pragma once

#include <stdint.h>

class ZoomMSDevice {

private:
    int8_t              _currentPatch = 0;
    char                _currentPatchName[11] = {0};
    uint8_t             _patchLen;
    uint8_t             _deviceID;
    bool                _tunerEnabled = false;


public:
    ZoomMSDevice();
};