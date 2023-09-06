
#include "zoom-multistomp.hpp"

// id request
uint8_t 			ID_PAK[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
// set editor mode on / off
uint8_t 			EM_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x50 /* 0x50: on - 0x51: off */, 0xf7 };
// get patch index
uint8_t 			PI_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x33, 0xf7 };
// get patch data
uint8_t 			PD_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x29, 0xf7 };
// set tuner mode on / off
uint8_t 			TU_PAK[] = { 0xb0, 0x4a, 0x00 /* 0x41: on - 0x0: off */ };
// program change
uint8_t 			PC_PAK[] = { 0xc0, 0x00 /* program number */ };



// ----------------------------------------------------------------------------
// ZOOM DEVICE HELPERS
// ----------------------------------------------------------------------------
void ZoomMSDevice::incPatch(int8_t aOffset) {
    _currentPatch = _currentPatch + aOffset;
    _currentPatch = _currentPatch > (DEV_MAX_PATCHES - 1) ? 0 : _currentPatch;
    _currentPatch = _currentPatch < 0 ? (DEV_MAX_PATCHES -1) : _currentPatch;

    sendPatch();
    requestPatchData();
    updateDisplay();
}


void ZoomMSDevice::sendBytes(uint8_t * aBytes, const __FlashStringHelper * aMessage = NULL) {
    dprintln(aMessage);
    _usb.Task();
    _midi.SendData(aBytes);
}


void ZoomMSDevice::readResponse(bool aIsSysEx = true) {
    uint16_t recv_read = 0;
    uint16_t recv_count = 0;
    uint8_t rcode = 0;

    delay(200); // TODO: fine-tune this
    dprintln(F("readResponse"));
            
    _usb.Task();
    do {
        rcode = _midi.RecvData(&recv_read, (uint8_t *)(_readBuffer + recv_count));
		dprintln(F("rcode"));
        dprintln(rcode);

        if(rcode == 0) {
            recv_count += recv_read;
            dprintln(F("recv_read"));
            dprintln(recv_read);
            dprintln(F("recv_count"));
            dprintln(recv_count);
        }
    } while(/*recv_count < MIDI_MAX_SYSEX_SIZE &&*/ rcode == 0);

    // debug
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


// ----------------------------------------------------------------------------
// ZOOM DEVICE I/O
// ----------------------------------------------------------------------------
void ZoomMSDevice() {
    _usb.Init();

	updateDisplay(F(" USB INIT "), 0, 0);
        
    int state = 0; 
    int rcode = 0;
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

    char fw_version[5] = {0};
    fw_version[0] = _readBuffer[10];
    fw_version[1] = _readBuffer[11];
    fw_version[2] = _readBuffer[12];
    fw_version[3] = _readBuffer[13];

    dprint(F("DEVICE ID: 0x"));
    hprintln(_deviceID);

    _patchLen = 0;
    const __FlashStringHelper * device_name = DEV_NAME_INVALID;
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

    _display.clearDisplay();
    _display.setCursor(0, 0);
    _display.println(device_name);
    _display.setCursor(0, 16);
    _display.println(fw_version);
    _display.display();

    requestPatchIndex();
    enableEditorMode(true);
    requestPatchData();
    delay(1500);
}


void ZoomMSDevice::enableEditorMode(bool aEnable) {
	EM_PAK[3] = _deviceID;
	EM_PAK[4] = aEnable ? 0x50 : 0x51;
    sendBytes(EM_PAK, F("EDITOR ON"));
}


void ZoomMSDevice::requestPatchIndex() {
    PI_PAK[3] = _deviceID;
    sendBytes(PI_PAK, F("REQ PATCH INDEX"));
    readResponse();
    _currentPatch = _readBuffer[7];
    dprint(F("Current patch: "));
    dprintln(_currentPatch);
}


void ZoomMSDevice::requestPatchData() {
    PD_PAK[3] = _deviceID;
    sendBytes(PD_PAK, F("REQ PATCH DATA"));
    readResponse();

    _currentPatchName[0] = _readBuffer[_patchLen - 14];
    _currentPatchName[1] = _readBuffer[_patchLen - 12];
    _currentPatchName[2] = _readBuffer[_patchLen - 11];
    _currentPatchName[3] = _readBuffer[_patchLen - 10];
    _currentPatchName[4] = _readBuffer[_patchLen - 9];
    _currentPatchName[5] = _readBuffer[_patchLen - 8];
    _currentPatchName[6] = _readBuffer[_patchLen - 7];
    _currentPatchName[7] = _readBuffer[_patchLen - 6];
    _currentPatchName[8] = _readBuffer[_patchLen - 4];
    _currentPatchName[9] = _readBuffer[_patchLen - 3];
    _currentPatchName[10] = '\0';

    dprint(F("Name: "));
    dprintln(_currentPatchName);
}


void ZoomMSDevice::toggleTuner() {
	_tunerEnabled = !_tunerEnabled;
	TU_PAK[2] = _tunerEnabled ? 0x41 : 0x0;
    sendBytes(TU_PAK);
	if(_tunerEnabled) {
    	updateDisplay(F(" TUNER ON "), 0, 0);
	}
    else {
    	updateDisplay();
    	// this flag will prevent patch scrolling 
    	// if buttons are released not quite simultaneously
    	_cancelScroll = true;
    }
}


void ZoomMSDevice::sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(_currentPatch);

    _usb.Task();
    PC_PAK[1] = _currentPatch;
    _midi.SendData(PC_PAK);
}

