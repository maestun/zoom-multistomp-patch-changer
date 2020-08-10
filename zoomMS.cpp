#include "ZoomMS.h"

#define MAX_PATCHES                 (50)
#define MIDI_BYTE_PROGRAM_CHANGE    (0xC0)

#define OLED_RESET                  -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH                128 // OLED display width, in pixels
#define SCREEN_HEIGHT               32 // OLED display height, in pixels

USB       gUsb;
USBH_MIDI gMidi(&gUsb);

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

uint8_t getPatchDataLength(uint8_t aDeviceID) {
    if(aDeviceID == MS_50G || aDeviceID == MS_70CDR) {
        return 146;
    }
    else if(aDeviceID == MS_60B) {
        return 105;
    }
    return 0;
}


ZoomMS::ZoomMS(uint8_t aPrevPin, uint8_t aNextPin, uint16_t aLongpressDelayMS, uint16_t aAutoCycleDelayMS) {

    initUSB();
    loadPatch();
    initDisplay();
    _cycleTS = 0;
    _cycleMS = aAutoCycleDelayMS;
    _prevPin = aPrevPin;
    _nextPin = aNextPin;
    _prevButton = new Button(_prevPin, aLongpressDelayMS, this);
    _nextButton = new Button(_nextPin, aLongpressDelayMS, this);
    sendPatch();
    updateDisplay();
}


void ZoomMS::initUSB() {
    gUsb.Init();

    dprintln(F("WILL INIT USB"));
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

    dprintln(F("USB RUNNING !"));
    dprint(F("Exit loop wait time (ms): "));
    dprintln(wait_ms);
    wait_ms = 0;

    // identify
    uint8_t pak[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
    gUsb.Task();
    rcode = gMidi.SendData(pak);
    dprint(F("Request ID returns "));
    dprintln(rcode);

    // wait for response    
    uint8_t recv[MIDI_EVENT_PACKET_SIZE];
    uint16_t recv_count;

    gUsb.Task();
    rcode = gMidi.RecvData(&recv_count, recv);

    //data check
    dprintln("Data: ");
    for(int i = 0; i < MIDI_EVENT_PACKET_SIZE; i++) {
        Serial.print(recv[i], HEX);
        dprint(" ");
    }
    dprintln("");
    dprint(F("RecvData rcode: "));
    dprintln(rcode);
    dprint(F("RecvData count: "));
    dprintln(recv_count);

    // parse get ID message
    _deviceID = recv[9];
    char fw_version[5] = {0};
    fw_version[0] = recv[14];
    fw_version[1] = recv[15];
    fw_version[2] = recv[17];
    fw_version[3] = recv[18];

    dprint(F("DEVICE ID: "));
    dprintln(_deviceID);

    dprint(F("DEVICE NAME: "));
    dprintln(getDeviceName(_deviceID));

    dprint(F("DEVICE FW: "));
    dprintln(fw_version);

    dprint(F("DEVICE PLEN: "));
    dprintln(getPatchDataLength(_deviceID));

    // _display->clearDisplay();
    // _display->setTextSize(2);
    // _display->setCursor(1, 1);
    // _display->setTextColor(SSD1306_WHITE);
    // _display->println(getDeviceName(_deviceID));
    // _display->setCursor(1, 16);
    // // _display->println(fw_version);
    // _display->display();

    delay(1000);
}


void ZoomMS::initDisplay() {
    _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if(!_display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        dprintln("SSD1306 allocation failed");
    }
    _display->clearDisplay();
    _display->setTextSize(2);
    _display->setCursor(1, 1);
    _display->setTextColor(SSD1306_WHITE);
    _display->println("INIT...");
    _display->display();
}


void ZoomMS::updateDisplay() {
    int dispPatch = _currentPatch + 1;
    _display->clearDisplay();

    dprint(F("CURRENT PATCH: "));
    dprintln(dispPatch);

    _display->setTextSize(4);
    _display->setTextColor(SSD1306_WHITE);
    _display->setCursor(76, 1);

    if (dispPatch < 10) {
        _display->print("0");  
    }
    _display->print(dispPatch);

    _display->setTextSize(2);
    _display->setCursor(1, 1);

    _display->println("PATCH");
    _display->display();
}


// send patch number thru MIDI
void ZoomMS::sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(_currentPatch);

    gUsb.Task();
    int state = gUsb.getUsbTaskState();
    if(state == USB_STATE_RUNNING) {
        byte pak[2] = {MIDI_BYTE_PROGRAM_CHANGE, _currentPatch};
        gMidi.SendData(pak);
    }
    else {
        dprint("USB STATE: ");
        dprintln(state);
    }

    /*
    midiEventPacket_t pak = {0x0c, 0xc0 | 0, _currentPatch, 0x00};
    MidiUSB.sendMIDI(pak);
    MidiUSB.flush();
    */
    delay(5);
}


// load last patch number from eeprom
void ZoomMS::loadPatch() {
    byte p = EEPROM.read(0);
    if (p > MAX_PATCHES) {
        p = 0;
    }
    _currentPatch = p;
}


// save current patch number to eeprom
void ZoomMS::savePatch() {
    EEPROM.write(0, _currentPatch);
}


void ZoomMS::updatePatch(int inc) {
    _currentPatch = _currentPatch + inc;
    _currentPatch = _currentPatch > (MAX_PATCHES - 1) ? 0 : _currentPatch;
    _currentPatch = _currentPatch < 0 ? (MAX_PATCHES -1) : _currentPatch;
}


// callback triggered on button event
void ZoomMS::onButtonEvent(uint8_t aPin, EButtonScanResult aResult) {
    bool prev = !!(aPin == _prevPin);
    
    dprint(prev ? "PREV " : "NEXT ");    

    if(aResult == EButtonDown) {
        // button down
        dprintln(F("DOWN"));
                
        int inc = (prev ? -1 : 1);
        updatePatch(inc);
        sendPatch();
        updateDisplay();
        savePatch();
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
            // update patch display but do not send !!
            int inc = (prev ? -1 : 1);
            updatePatch(inc);
            updateDisplay();
        }
    }
    else if(aResult == EButtonUnlongpress) {
        // button released from longpress
        dprintln(F("UNLONG"));
        _cycleTS = 0;
        sendPatch();
        updateDisplay();
        savePatch();
    }
    else if(aResult == EButtonClick) {
        // button clicked
        dprintln(F("CLICK"));
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
