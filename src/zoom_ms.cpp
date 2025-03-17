#include "zoom_ms.h"
#include "debug.h"

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

USB         _usb;
USBH_MIDI   _midi(&_usb);

// id request
uint8_t 			ID_PAK[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
// set editor mode on / off
uint8_t 			EM_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x50 /* 0x50: on - 0x51: off */, 0xf7 };
// get patch index
uint8_t 			PI_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x33, 0xf7 };
// get patch data
uint8_t 			PD_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x29, 0xf7};
// set tuner mode on / off
uint8_t 			TU_PAK[] = { 0xb0, 0x4a, 0x00 /* 0x41: on - 0x0: off */ };
// program change
uint8_t 			PC_PAK[] = { 0xc0, 0x00 /* program number */ };
// bypass effect
uint8_t 			BP_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x31, 0x00 /* effect slot number */, 0x00, 0x00 /* 1: on, 0: off */, 0x00, 0x00, 0xf7 };

void ZoomMSDevice::debugReadBuffer(const __FlashStringHelper * aMessage, bool aIsSysEx) {
    dprintln(aMessage);
    int i = 0;
    for(i = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        dprint("0x");
        if (_readBuffer[i] < 16) 
            dprint("0");
        hprint(_readBuffer[i]);
        dprint(", ");
        if(aIsSysEx && _readBuffer[i] == 0xf7) {
            i++;
            break;
        }
    }

    dprintln("");
    dprint(F("NUM BYTES: "));
    dprintln(i);
}


void ZoomMSDevice::readResponse(bool aIsSysEx) {
    uint16_t recv_read = 0;
    uint16_t recv_count = 0;
    uint8_t rcode = 0;

    delay(200); // TODO: fine-tune this
    dprintln(F("readResponse"));
            
    _usb.Task();
    do {
        rcode = _midi.RecvData(&recv_read, (uint8_t *)(_readBuffer + recv_count));
		// dprintln(F("rcode"));
        // dprintln(rcode);

        if(rcode == 0) {
            recv_count += recv_read;
            // dprintln(F("recv_read"));
            // dprintln(recv_read);
            // dprintln(F("recv_count"));
            // dprintln(recv_count);
        }
    } while(/*recv_count < MIDI_MAX_SYSEX_SIZE &&*/ rcode == 0);
    
    // debugReadBuffer(F("RAW READ: "), true);

    // remove MIDI packet's 1st byte
    for(int i = 0, j = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        // TODO: stop at 0xf7 when sysex
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
    }  

    // debugReadBuffer(F("SYSEX READ: "), true);
}

void ZoomMSDevice::sendBytes(uint8_t * aBytes, const __FlashStringHelper * aMessage) {
    dprintln(aMessage);
    _usb.Task();
    _midi.SendData(aBytes);
}

void ZoomMSDevice::enableEditorMode(bool aEnable) {
	EM_PAK[4] = aEnable ? 0x50 : 0x51;
    sendBytes(EM_PAK, F("EDITOR ON"));
}

void ZoomMSDevice::requestPatchIndex() {
    sendBytes(PI_PAK, F("REQ PATCH INDEX"));
    readResponse();
    patch_index = _readBuffer[7];
    dprint(F("Current patch: "));
    dprintln(patch_index);
}

void ZoomMSDevice::requestPatchData() {
    sendBytes(PD_PAK, F("REQ PATCH DATA"));
    readResponse();

    patch_name[0] = _readBuffer[_patchLen - 14];
    patch_name[1] = _readBuffer[_patchLen - 12];
    patch_name[2] = _readBuffer[_patchLen - 11];
    patch_name[3] = _readBuffer[_patchLen - 10];
    patch_name[4] = _readBuffer[_patchLen - 9];
    patch_name[5] = _readBuffer[_patchLen - 8];
    patch_name[6] = _readBuffer[_patchLen - 7];
    patch_name[7] = _readBuffer[_patchLen - 6];
    patch_name[8] = _readBuffer[_patchLen - 4];
    patch_name[9] = _readBuffer[_patchLen - 3];
    patch_name[10] = '\0';

    dprint(F("Name: "));
    dprintln(patch_name);
}

ZoomMSDevice::ZoomMSDevice() {

    uint8_t ret = _usb.Init();
    dprint(F("USB ret: "));
    dprintln(ret);

    int state = 0; 
    uint32_t wait_ms = 0;
    int inc_ms = 100;
    do {
        _usb.Task();
        state = _usb.getUsbTaskState();
        dprint(F("USB STATE: "));
        dprintln(state);
        delay(inc_ms);
        wait_ms += inc_ms;
    } while(state != USB_STATE_RUNNING);

    dprint(F("USB RUNNING "));
    dprint(F(" - Exit loop wait time ms: "));
    dprintln(wait_ms);
    
    // identify
    sendBytes(ID_PAK, F("REQ ID"));
    readResponse();

    _deviceID = _readBuffer[6];
    if(_deviceID == DEV_ID_MS_50G || _deviceID ==DEV_ID_MS_70CDR) {
        _patchLen = 146;
    }
    else {
        _patchLen = 105;
    }
    BP_PAK[3] = _deviceID;
    EM_PAK[3] = _deviceID;
    PI_PAK[3] = _deviceID;
    PD_PAK[3] = _deviceID;

    // char fw_version[5] = {0};
    fw_version[0] = _readBuffer[10];
    fw_version[1] = _readBuffer[11];
    fw_version[2] = _readBuffer[12];
    fw_version[3] = _readBuffer[13];

    dprint(F("DEVICE ID: 0x"));
    hprintln(_deviceID);

    _patchLen = 0;
    device_name = DEV_NAME_INVALID;
    switch(_deviceID) {
	case DEV_ID_MS_50G:
		_patchLen = DEV_PLEN_MS_50G;
		device_name = DEV_NAME_MS_50G;
		break;
	case DEV_ID_MS_70CDR:
		_patchLen = DEV_PLEN_MS_70CDR;
		device_name = DEV_NAME_MS_70CDR;
		break;
	case DEV_ID_MS_60B:
		_patchLen = DEV_PLEN_MS_60B;
		device_name = DEV_NAME_MS_60G;
		break;
	default:
		break;
    }

    dprint(F("DEVICE NAME: "));
    dprintln(device_name);

    dprint(F("DEVICE FW: "));
    dprintln(fw_version);

    dprint(F("PATCH LEN: "));
    dprintln(_patchLen);

    requestPatchIndex();
    enableEditorMode(true);
    requestPatchData();
    delay(500);

    bypassed = _readBuffer[6] & 0x1;
    dprint(F("Active: "));
    dprintln(bypassed ? F("YES") : F("NO"));
}

void ZoomMSDevice::sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(patch_index);

    _usb.Task();
    PC_PAK[1] = patch_index;
    _midi.SendData(PC_PAK);
}

void ZoomMSDevice::incPatch(int8_t aOffset) {
    patch_index = patch_index + aOffset;
    patch_index = patch_index > (DEV_MAX_PATCHES - 1) ? 0 : patch_index;
    patch_index = patch_index < 0 ? (DEV_MAX_PATCHES -1) : patch_index;

    sendPatch();
    requestPatchData();
}

void ZoomMSDevice::toggleFullBypass() {
	bypassed = !bypassed;
    
    BP_PAK[7] = bypassed ? 0x1 : 0x0;
    for (int i = 0; i < DEV_MAX_FX_PER_PATCH; i++) {
        BP_PAK[5] = i;
        sendBytes(BP_PAK);
    }
}

void ZoomMSDevice::toggleBypass() {
	bypassed = !bypassed;
    BP_PAK[5] = 0; // consider the 1st slot to be the line selector
	BP_PAK[7] = bypassed ? 0x1 : 0x0;
    sendBytes(BP_PAK);
}


void ZoomMSDevice::toggleTuner() {
	tuner_enabled = !tuner_enabled;
	TU_PAK[2] = tuner_enabled ? 0x41 : 0x0;
    sendBytes(TU_PAK);
}