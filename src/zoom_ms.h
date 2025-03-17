#pragma once

#include <Usb.h>
#include <usbh_midi.h>

// Zoom device characteristics, don't change
#define DEV_MAX_PATCHES             (50)
#define DEV_ID_MS_50G 				(0x58)
#define DEV_ID_MS_70CDR 			(0x61)
#define DEV_ID_MS_60B 				(0x5f)
#define DEV_PLEN_MS_50G 			(146)
#define DEV_PLEN_MS_70CDR 			(146)
#define DEV_PLEN_MS_60B 			(105)
#define DEV_NAME_MS_50G				F("MS-50G")
#define DEV_NAME_MS_70CDR			F("MS-70CDR")
#define DEV_NAME_MS_60G				F("MS-60G")
#define DEV_NAME_INVALID			F("INVALID")
#define DEV_MAX_FX_PER_PATCH        (5)


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