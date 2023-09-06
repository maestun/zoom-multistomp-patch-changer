#pragma once

#include <stdint.h>
#include <Usb.h>
#include <usbh_midi.h>

extern "C" {
    typedef void (*cb_t)(void);
    typedef void (*enable_cb_t)(bool);
}

class ZoomMSDeviceListener {
public:
    virtual void onPatchInc() = 0;
    virtual void onDeviceReady(const __FlashStringHelper * device_name, 
                               char * fw_version) = 0;
    virtual void onTunerEnabled(bool aEnabled) = 0;
};

class ZoomMSDevice {

private:
    int8_t              _currentPatch = 0;
    char                _currentPatchName[11] = {0};
    uint8_t             _patchLen;
    uint8_t             _deviceID;
    bool                _tunerEnabled = false;
    bool 				_cancelScroll = false;


    USB         		_usb;
    USBH_MIDI   		_midi;
    uint8_t 			_readBuffer[MIDI_MAX_SYSEX_SIZE] = {0};

    ZoomMSDeviceListener& _listener;

    void sendBytes(uint8_t * aBytes, const __FlashStringHelper * aMessage = NULL);
    void readResponse(bool aIsSysEx = true);
    void requestPatchIndex();
    void requestPatchData();
    void debugReadBuffer(const __FlashStringHelper * aMessage, bool aIsSysEx);

public:
    ZoomMSDevice(ZoomMSDeviceListener& listener);

    void incPatch(int8_t aOffset);
    void sendPatch();
    void toggleTuner();
    void enableTuner(bool aEnable);
    bool tunerEnabled();
    void enableEditorMode(bool aEnable);
};