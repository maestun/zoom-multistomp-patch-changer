#pragma once

#include <Usb.h>
#include <usbh_midi.h>

class ZoomMSDevice {

private:
    uint8_t 	        _readBuffer[MIDI_MAX_SYSEX_SIZE] = {0};
    uint8_t             _patchLen;
    uint8_t             _deviceID;
    
public:
    bool                        tuner_enabled = false;
    bool                        bypassed = false;
    int8_t                      patch_index = 0;
    char                        patch_name[11] = {0};
    const __FlashStringHelper * device_name;
    char                        fw_version[5] = {0};

private:
    void debugReadBuffer(const __FlashStringHelper * aMessage, bool aIsSysEx);
    void readResponse(bool aIsSysEx = true);
    void sendBytes(uint8_t * aBytes, const __FlashStringHelper * aMessage = NULL);
    void requestPatchIndex();
    void requestPatchData();
    void sendPatch();
    
public:
    ZoomMSDevice();
    void enableEditorMode(bool aEnable);
    // bool is_bypassed();
    // bool is_tuner_enabled();
    // int patch_index();
    // char* patch_name();
    void incPatch(int8_t aOffset);
    void toggleBypass();
    void toggleFullBypass();
    void toggleTuner();
};