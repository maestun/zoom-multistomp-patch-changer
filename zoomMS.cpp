#include "ZoomMS.h"

#define MAX_PATCHES                 (50)
#define MIDI_BYTE_PROGRAM_CHANGE    (0xC0)

#define OLED_RESET                  -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH                128 // OLED display width, in pixels
#define SCREEN_HEIGHT               32 // OLED display height, in pixels

USB         gUsb;
USBH_MIDI   gMidi(&gUsb);
uint8_t     gReadBuffer[MIDI_MAX_SYSEX_SIZE] = {0};

enum EZoomDevice {
    MS_50G = 0x58,
    MS_70CDR = 0x61,
    MS_60B = 0x5f
};

const __FlashStringHelper * getDeviceName(uint8_t aDeviceID) {
    if(aDeviceID == MS_50G) {
        return F("MS-50G");
    }
    else if(aDeviceID == MS_70CDR) {
        return F("MS-70CDR");
    }
    else if(aDeviceID == MS_60B) {
        return F("MS-60B");
    }
    return F("INVALID");
}


ZoomMS::ZoomMS(uint8_t aPrevPin, uint8_t aNextPin, uint16_t aLongpressDelayMS, uint16_t aAutoCycleDelayMS) {
    
    _cycleTS = 0;
    _cycleMS = aAutoCycleDelayMS;
    _prevPin = aPrevPin;
    _nextPin = aNextPin;
    _prevButton = new Button(_prevPin, aLongpressDelayMS, this);
    _nextButton = new Button(_nextPin, aLongpressDelayMS, this);

    initDisplay();
    initDevice();
    updateDisplay();
}


void debugReadBuffer(char * aMessage, bool aIsSysEx) {
#ifdef _DEBUG
    dprintln(aMessage);
    int i = 0;
    for(i = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        dprint("0x");         
        hprint(gReadBuffer[i]);
        dprint(", ");
        if(aIsSysEx && gReadBuffer[i] == 0xf7) {
            i++;
            break;
        }
    }
    dprintln("");
    dprint("NUM BYTES: ");
    dprintln(i);
#endif
}


void ZoomMS::sendBytes(uint8_t * aBytes, char * aMessage) {
    dprintln(aMessage);
    gUsb.Task();
    uint8_t rcode = gMidi.SendData(aBytes);
    dprint("rcode: ");
    dprintln(rcode);
}


void ZoomMS::readResponse() {
   
    uint16_t recv_read = 0;
    uint16_t recv_count = 0;
    uint8_t rcode = 0;

    delay(100);
    dprintln("readResponse");
    
    gUsb.Task();
    do {
        rcode = gMidi.RecvData(&recv_read, (uint8_t *)(gReadBuffer + recv_count));
        if(rcode == 0) {
            recv_count += recv_read;
            dprintln("rcode");
            dprintln(rcode);
            dprintln("recv_read");
            dprintln(recv_read);
            dprintln("recv_count");
            dprintln(recv_count);
        }
        else {
            dprintln("*** BAD rcode");
            dprintln(rcode);
        }
    } while(recv_count < MIDI_MAX_SYSEX_SIZE && rcode == 0);

    // debug
    debugReadBuffer("RAW READ: ", true);

    // remove MIDI packet's 1st byte
    for(int i = 0, j = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        // TODO: stop at 0xf7 when sysex
        gReadBuffer[j++] = gReadBuffer[++i];
        gReadBuffer[j++] = gReadBuffer[++i];
        gReadBuffer[j++] = gReadBuffer[++i];
    }

    debugReadBuffer("SYSEX READ: ", true);
    
    dprintln("--> readResponse DONE");
}


void ZoomMS::initDevice() {
    gUsb.Init();

    dprintln(F("INIT USB"));
    int state = 0; 
    int rcode = 0;
    uint32_t wait_ms = 0;
    int inc_ms = 100;
    do {
        gUsb.Task();
        state = gUsb.getUsbTaskState();
        dprint(F("USB STATE: "));
        dprintln(state);
        delay(inc_ms);
        wait_ms += inc_ms;
    } while(state != USB_STATE_RUNNING);

    dprint(F("USB RUNNING "));
    dprint(F(" - Exit loop wait time ms: "));
    dprintln(wait_ms);
    
    // identify
    uint8_t pak[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
    sendBytes(pak, "REQ ID");
    readResponse();

    _deviceID = gReadBuffer[6];
    if(_deviceID == MS_50G || _deviceID == MS_70CDR) {
        _patchLen = 146;
    }
    else {
        _patchLen = 105;
    }

    char fw_version[5] = {0};
    fw_version[0] = gReadBuffer[10];
    fw_version[1] = gReadBuffer[11];
    fw_version[2] = gReadBuffer[12];
    fw_version[3] = gReadBuffer[13];

    dprint(F("DEVICE ID: 0x"));
    hprintln(_deviceID);

    dprint(F("DEVICE NAME: "));
    dprintln(getDeviceName(_deviceID));

    dprint(F("DEVICE FW: "));
    dprintln(fw_version);

    dprint(F("PATCH LEN: "));
    dprintln(_patchLen);

    _display->clearDisplay();
    _display->setTextSize(2);
    _display->setCursor(0, 0);
    _display->setTextColor(SSD1306_WHITE);
    _display->println(getDeviceName(_deviceID));
    _display->setCursor(0, 16);
    _display->println(fw_version);
    _display->display();

    uint8_t pd_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x33, 0xf7 };
    sendBytes(pd_pak, "REQ PATCH INDEX");

    readResponse();
    _currentPatch = gReadBuffer[7];
    dprint(F("Current patch: "));
    dprintln(_currentPatch);

    uint8_t em_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x50, 0xf7 };
    sendBytes(em_pak, "SET EDITOR ON");
    requestPatchData();
    
    delay(1500);
}


void ZoomMS::requestPatchData() {

    uint8_t pd_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x29, 0xf7 };
    sendBytes(pd_pak, "REQ PATCH DATA");
    readResponse();

    _currentPatchName[0] = gReadBuffer[_patchLen - 14];
    _currentPatchName[1] = gReadBuffer[_patchLen - 12];
    _currentPatchName[2] = gReadBuffer[_patchLen - 11];
    _currentPatchName[3] = gReadBuffer[_patchLen - 10];
    _currentPatchName[4] = gReadBuffer[_patchLen - 9];
    _currentPatchName[5] = gReadBuffer[_patchLen - 8];
    _currentPatchName[6] = gReadBuffer[_patchLen - 7];
    _currentPatchName[7] = gReadBuffer[_patchLen - 6];
    _currentPatchName[8] = gReadBuffer[_patchLen - 4];
    _currentPatchName[9] = gReadBuffer[_patchLen - 3];
    _currentPatchName[10] = '\0';

    dprint("Name: ");
    dprintln(_currentPatchName);
}


// send patch number thru MIDI
void ZoomMS::sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(_currentPatch);

    gUsb.Task();
    uint8_t pak[2] = {MIDI_BYTE_PROGRAM_CHANGE, _currentPatch};
    gMidi.SendData(pak);
}


void ZoomMS::initDisplay() {
    _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if(!_display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        dprintln("SSD1306 allocation failed");
    }
    _display->clearDisplay();
    _display->setTextSize(2);
    _display->setCursor(0, 0);
    _display->setTextColor(SSD1306_WHITE);
    _display->println("USB INIT");
    _display->display();
}


void ZoomMS::updateDisplay() {
    int dispPatch = _currentPatch + 1;
    _display->clearDisplay();

    _display->setTextColor(SSD1306_WHITE);
    _display->setTextSize(2);
    _display->setCursor(0, 0);
    _display->println(_currentPatchName);
    _display->setCursor(100, 16);
    if (dispPatch < 10) {
        _display->print("0");  
    }
    _display->println(dispPatch);
    _display->display();
}


void ZoomMS::incPatch(bool aIsPrev) {
    _currentPatch = _currentPatch + (aIsPrev ? -1 : 1);
    _currentPatch = _currentPatch > (MAX_PATCHES - 1) ? 0 : _currentPatch;
    _currentPatch = _currentPatch < 0 ? (MAX_PATCHES -1) : _currentPatch;

    sendPatch();
    requestPatchData();
    updateDisplay();
}


// callback triggered on button event
void ZoomMS::onButtonEvent(uint8_t aPin, EButtonScanResult aResult) {
    bool isPrev = !!(aPin == _prevPin);
    
    dprint(isPrev ? "PREV " : "NEXT ");

    if(aResult == EButtonDown) {
        // button down
        dprintln(F("DOWN"));

    }
    else if(aResult == EButtonLongpress) {
        // button longpressed
        dprintln(F("LONG"));
    }
    else if(aResult == EButtonHold) {
        // button held down
        dprintln(F("HOLD"));
        uint32_t ts = millis();
        if((ts - _cycleTS) >= _cycleMS) {
            _cycleTS = ts;
            incPatch(isPrev);
            // sendPatch();
            // requestPatchData();
            // updateDisplay();
        }
    }
    else if(aResult == EButtonUnlongpress) {
        // button released from longpress
        dprintln(F("UNLONG"));
        _cycleTS = 0;
        // sendPatch();
        // requestPatchData();
        // updateDisplay();
    }
    else if(aResult == EButtonClick) {
        // button clicked
        dprintln(F("CLICK"));
        incPatch(isPrev);
        // sendPatch();
        // requestPatchData();
        // updateDisplay();
    }
    else if(aResult == EButtonUp) {
        // button released from shortpress: ignore
        dprintln(F("UP"));
    }
}


// call this in main loop
void ZoomMS::scan() {
    _prevButton->scan();
    _nextButton->scan();
}
