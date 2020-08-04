#include "zoomMS70.h"

#define MAX_PATCHES                 (50)
#define MIDI_BYTE_PROGRAM_CHANGE    (0xC0)

#define OLED_RESET                  -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH                128 // OLED display width, in pixels
#define SCREEN_HEIGHT               32 // OLED display height, in pixels

USB       gUsb;
USBH_MIDI gMidi(&gUsb);

ZoomMS70::ZoomMS70(uint8_t aPrevPin, uint8_t aNextPin, uint16_t aLongpressDelayMS, uint16_t aAutoCycleDelayMS) {

    gUsb.Init();
    
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


void ZoomMS70::initDisplay() {
  _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if(!_display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        dprintln("SSD1306 allocation failed");
    }
    _display->clearDisplay();
}


void ZoomMS70::updateDisplay() {
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
void ZoomMS70::sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(_currentPatch);

    gUsb.Task();
    if(gUsb.getUsbTaskState() == USB_STATE_RUNNING) {
        
        byte pak[2] = {MIDI_BYTE_PROGRAM_CHANGE, _currentPatch};
        gMidi.SendData(pak);
        delay(50);
    }

    /*
    midiEventPacket_t pak = {0x0c, 0xc0 | 0, _currentPatch, 0x00};
    MidiUSB.sendMIDI(pak);
    MidiUSB.flush();
    */
    delay(5);
}


// load last patch number from eeprom
void ZoomMS70::loadPatch() {
    byte p = EEPROM.read(0);
    if (p > MAX_PATCHES) {
        p = 0;
    }
    _currentPatch = p;
}


// save current patch number to eeprom
void ZoomMS70::savePatch() {
    EEPROM.write(0, _currentPatch);
}


void ZoomMS70::updatePatch(int inc) {
    _currentPatch = _currentPatch + inc;
    _currentPatch = _currentPatch > (MAX_PATCHES - 1) ? 0 : _currentPatch;
    _currentPatch = _currentPatch < 0 ? (MAX_PATCHES -1) : _currentPatch;
}


// callback triggered on button event
void ZoomMS70::onButtonEvent(uint8_t aPin, EButtonScanResult aResult) {
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
void ZoomMS70::scan() {
    _prevButton->scan();
    _nextButton->scan();
}
