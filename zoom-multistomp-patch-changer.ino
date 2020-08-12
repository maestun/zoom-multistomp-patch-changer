// ----------------------------------------------------------------------------
// Zoom MultiStomp patch selector
// 
// Increment / decrement patch numbers (up to 50) using your feet.
// A longpress will trigger the cycling through the patches.
// A longpress on both buttons will enable / disable the tuner.
//
// Libraries used:
// - USB Host Shield Library v2.0 ( https://github.com/felis/USB_Host_Shield_2.0 )
// - Adafruit_SSD1306 ( https://github.com/adafruit/Adafruit_SSD1306 )
// - OneButton ( https://github.com/mathertel/OneButton )
// ----------------------------------------------------------------------------
#include "debug.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Usb.h>
#include <usbh_midi.h>
#include <OneButton.h>

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

// footswitch pins for patch next / prev with internal pullup resistors
#define PIN_BUTTON_PREV             (A1)
#define PIN_BUTTON_NEXT             (A2)

// delay (in milliseconds) between two patch increments while scrolling
#define AUTOCYCLE_DELAY_MS          (250)

// display stuff
#define PIN_OLED_RESET              (-1)
#define SCREEN_WIDTH                (128)
#define SCREEN_HEIGHT               (32)


// ----------------------------------------------------------------------------
// GLOBALS
// ----------------------------------------------------------------------------
// communication stuff
USB         		_usb;
USBH_MIDI   		_midi(&_usb);
uint8_t 			_readBuffer[MIDI_MAX_SYSEX_SIZE] = {0};

// button stuff
uint32_t  			_cycleTS;
uint16_t  			_cycleMS = 200;
bool      			_btNextDown = false;
bool      			_btPrevDown = false;
bool      			_isScrolling = false;
OneButton 			_btNext(A1, true);
OneButton 			_btPrev(A2, true);

// display stuff
Adafruit_SSD1306 	_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_OLED_RESET);

// device stuff
int8_t              _currentPatch;
char                _currentPatchName[11];
uint8_t             _patchLen;
uint8_t             _deviceID;
bool                _tunerEnabled;


// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
void setup() {
    dprintinit(9600);

    // peripheral init
    initDisplay();
    initDevice();
    updateDisplay();

    // button init
    _btNext.attachPressStart([]() {
        _btNextDown = true;
    });
    _btNext.attachClick(onNextClicked);
    _btNext.attachLongPressStart(onNextLongStart);
    _btNext.attachDuringLongPress(onNextLongHold);
    _btNext.attachLongPressStop(onNextLongStop);

    _btPrev.attachPressStart([]() {
        _btPrevDown = true;
    });
    _btPrev.attachClick(onPrevClicked);
    _btPrev.attachLongPressStart(onPrevLongStart);
    _btPrev.attachDuringLongPress(onPrevLongHold);  
    _btPrev.attachLongPressStop(onPrevLongStop);
    
}


void loop() {
    _btPrev.tick();
    _btNext.tick();
}


// ----------------------------------------------------------------------------
// HELPERS
// ----------------------------------------------------------------------------
void debugReadBuffer(const __FlashStringHelper * aMessage, bool aIsSysEx) {
#ifdef _DEBUG
    dprintln(aMessage);
    int i = 0;
    for(i = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        dprint("0x");         
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
#endif
}


// ----------------------------------------------------------------------------
// ZOOM DEVICE HELPERS
// ----------------------------------------------------------------------------
void incPatch(bool aIsPrev) {
    _currentPatch = _currentPatch + (aIsPrev ? -1 : 1);
    _currentPatch = _currentPatch > (DEV_MAX_PATCHES - 1) ? 0 : _currentPatch;
    _currentPatch = _currentPatch < 0 ? (DEV_MAX_PATCHES -1) : _currentPatch;

    sendPatch();
    requestPatchData();
    updateDisplay();
}


void sendBytes(uint8_t * aBytes, const __FlashStringHelper * aMessage) {
    dprintln(aMessage);
    _usb.Task();
    uint8_t rcode = _midi.SendData(aBytes);
    dprint(F("sendBytes rcode: "));
    dprintln(rcode);
}


void readResponse() {
    uint16_t recv_read = 0;
    uint16_t recv_count = 0;
    uint8_t rcode = 0;

    delay(100); // TODO: fine-tune this
    dprintln(F("readResponse"));
    
    _usb.Task();
    do {
        rcode = _midi.RecvData(&recv_read, (uint8_t *)(_readBuffer + recv_count));
        if(rcode == 0) {
            recv_count += recv_read;
            dprintln(F("rcode"));
            dprintln(rcode);
            dprintln(F("recv_read"));
            dprintln(recv_read);
            dprintln(F("recv_count"));
            dprintln(recv_count);
        }
        else {
            dprintln(F("*** BAD rcode"));
            dprintln(rcode);
        }
    } while(recv_count < MIDI_MAX_SYSEX_SIZE && rcode == 0);

    // debug
    debugReadBuffer(F("RAW READ: "), true);

    // remove MIDI packet's 1st byte
    for(int i = 0, j = 0; i < MIDI_MAX_SYSEX_SIZE; i++) {
        // TODO: stop at 0xf7 when sysex
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
    }

    debugReadBuffer(F("SYSEX READ: "), true);
}



// ----------------------------------------------------------------------------
// ZOOM DEVICE I/O
// ----------------------------------------------------------------------------
void initDevice() {
    _usb.Init();

    dprintln(F("INIT USB"));
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
    uint8_t pak[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
    sendBytes(pak, F("REQ ID"));
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

    uint8_t pd_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x33, 0xf7 };
    sendBytes(pd_pak, F("REQ PATCH INDEX"));

    readResponse();
    _currentPatch = _readBuffer[7];
    dprint(F("Current patch: "));
    dprintln(_currentPatch);

    uint8_t em_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x50, 0xf7 };
    sendBytes(em_pak, F("SET EDITOR ON"));
    requestPatchData();
    
    _tunerEnabled = false;

    delay(1500);
}


void requestPatchData() {
    uint8_t pd_pak[] = { 0xf0, 0x52, 0x00, _deviceID, 0x29, 0xf7 };
    sendBytes(pd_pak, F("REQ PATCH DATA"));
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


void toggleTuner() {
	_tunerEnabled = !_tunerEnabled;
    uint8_t pak[] = { 0xb0, 0x4a, _tunerEnabled ? 0x41 : 0x0 };
    sendBytes(pak, _tunerEnabled ? F("TUNER ON") : F("TUNER OFF"));
}


// send patch number thru MIDI
void sendPatch() {
    // send current patch number MIDI over USB
    dprint(F("Sending patch: "));
    dprintln(_currentPatch);

    _usb.Task();
    uint8_t pak[] = {0xc0, _currentPatch};
    _midi.SendData(pak);
}


// ----------------------------------------------------------------------------
// DISPLAY HELPERS
// ----------------------------------------------------------------------------
void initDisplay() {
    if(!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        dprintln(F("SSD1306 allocation failed"));
    }
    _display.clearDisplay();
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    _display.println("USB INIT");
    _display.display();
}


void updateDisplay() {
    uint8_t p = _currentPatch + 1;
    _display.clearDisplay();
    _display.setCursor(0, 0);
    _display.println(_currentPatchName);
    _display.setCursor(100, 16);
    if (p < 10) {
        _display.print("0");  
    }
    _display.println(p);
    _display.display();
}


// ----------------------------------------------------------------------------
// BUTTON CALLBACKS
// ----------------------------------------------------------------------------
void onNextLongStart() {
    if(_btPrev.isLongPressed() == true && _isScrolling == false) {
        toggleTuner();
    }
}
void onPrevLongStart() {
    if(_btNext.isLongPressed() == true && _isScrolling == false) {
        toggleTuner();
    }
}


void onNextLongHold() {
  if(_tunerEnabled == false && 
     // _btPrev.isLongPressed() == false &&
     // _btNextDown == false &&
     _btPrevDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= _cycleMS) {
            _cycleTS = ts;
            // Serial.println("SCROLL NEXT...");
            incPatch(1);
        }
    }
} 
void onPrevLongHold() {
    if(_tunerEnabled == false && 
       // _btNext.isLongPressed() == false &&
       // _btPrevDown == false && 
       _btNextDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= _cycleMS) {
            _cycleTS = ts;
            // Serial.println("SCROLL PREV...");
            incPatch(-1);
        }
    }
}


void onNextLongStop() {
    _btNextDown = false;
    _isScrolling = false;
}
void onPrevLongStop() {
    _btPrevDown = false;
    _isScrolling = false;
}


void onNextClicked() {
    if(_tunerEnabled == false && !_isScrolling) {
        incPatch(1);
    }
    _btNextDown = false;
}
void onPrevClicked() {
    if(_tunerEnabled == false && !_isScrolling) {
        incPatch(-1);
    }
    _btPrevDown = false;
}
