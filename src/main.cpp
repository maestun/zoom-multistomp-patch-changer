// ----------------------------------------------------------------------------
// Zoom MultiStomp patch selector
// 
// Increment / decrement patch numbers (up to 50) using your feet.
// A longpress will trigger the cycling through the patches.
// A longpress on both buttons will enable / disable the tuner.
//
// Libraries used:
// - USB Host Shield Library v2.0 ( https://github.com/felis/USB_Host_Shield_2.0 )
// - OneButton ( https://github.com/mathertel/OneButton )
// ----------------------------------------------------------------------------

#include <Usb.h>
#include <usbh_midi.h>
#include <OneButton.h>

#include "debug.h"
#include "version.h"

#ifdef USE_OLED
#  include "display_oled.h"
#elif defined(USE_LCD)
#  include "display_lcd16x2.h"
#endif

#include "zoom_ms.h"

// footswitch pins for patch next / prev with internal pullup resistors
// LCD / OLED SDA -> A4
// LCD / OLED SCL -> A5
#define PIN_BUTTON_NEXT             (A1)
#define PIN_BUTTON_PREV             (A2)
#define PIN_BUTTON_BYPASS           (3)
#define PIN_LED_BYPASS              (10)

// delay (in milliseconds) between two patch increments while scrolling
#define AUTOCYCLE_DELAY_MS          (100)

// ----------------------------------------------------------------------------
// GLOBALS
// ----------------------------------------------------------------------------

// button stuff
uint32_t  			_cycleTS;
bool      			_btNextDown = false;
bool      			_btPrevDown = false;
bool      			_isScrolling = false;
bool 				_cancelScroll = false;
OneButton 			_btNext(PIN_BUTTON_NEXT, true);
OneButton 			_btPrev(PIN_BUTTON_PREV, true);
OneButton 			_btBypass(PIN_BUTTON_BYPASS, true);

// display stuff
IDisplay * display = nullptr;
ZoomMSDevice * zoom = nullptr;


// ----------------------------------------------------------------------------
// UTILS
// ----------------------------------------------------------------------------
void refreshUI() {
    display->showPatch(zoom->patch_index, zoom->patch_name);
    digitalWrite(PIN_LED_BYPASS, zoom->bypassed);
}
  
void toggleTuner() {
    zoom->toggleTuner();
	if(zoom->tuner_enabled) {
        display->clear();
        display->showString(F("    TUNER ON    "), 0, 0);
	}
    else {
        // display->showPatch(zoom->patch_index, zoom->patch_name);
        refreshUI();
    	// this flag will prevent patch scrolling 
    	// if buttons are released not quite simultaneously
    	_cancelScroll = true;
    }
}


// ----------------------------------------------------------------------------
// BUTTON CALLBACKS
// ----------------------------------------------------------------------------
// Toggle tuner only if:
// - we're not scrolling through the patches
// - prev+next buttons are being longpressed
void onNextLongStart() {
	_cancelScroll = false;
    if(_btPrev.isLongPressed() == true && _isScrolling == false) {
        toggleTuner();
    }
}
void onPrevLongStart() {
	_cancelScroll = false;
    if(_btNext.isLongPressed() == true && _isScrolling == false) {
        toggleTuner();
    }
}

// Scroll through patches only if:
// - tuner is disabled AND
// - the other button is not currently pressed
void onNextLongHold() {
  if(zoom->tuner_enabled == false && 
  	 _cancelScroll == false &&
     _btPrevDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= AUTOCYCLE_DELAY_MS) {
            _cycleTS = ts;
            zoom->incPatch(1);
            // display->showPatch(zoom->patch_index, zoom->patch_name);
            refreshUI();
        }
    }
} 
void onPrevLongHold() {
    if(zoom->tuner_enabled == false && 
       _cancelScroll == false &&
       _btNextDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= AUTOCYCLE_DELAY_MS) {
            _cycleTS = ts;
            zoom->incPatch(-1);
            // display->showPatch(zoom->patch_index, zoom->patch_name);
            refreshUI();
        }
    }
}

// Reset button state
void onNextLongStop() {
    _btNextDown = false;
    _isScrolling = false;
}
void onPrevLongStop() {
    _btPrevDown = false;
    _isScrolling = false;
}

// Single patch increment only if: 
// - tuner is disabled AND
// - we're not scrolling with the other button
void onNextClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        zoom->incPatch(1);
        // display->showPatch(zoom->patch_index, zoom->patch_name);
        refreshUI();
    }
    _btNextDown = false;
}
void onPrevClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        zoom->incPatch(-1);
        // display->showPatch(zoom->patch_index, zoom->patch_name);
        refreshUI();
    }
    _btPrevDown = false;
}

// Toggle bypass only if: 
// - tuner is disabled AND
// - we're not scrolling
void onBypassClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        dprintln(F("BYPASS"));
        zoom->toggleBypass();
        digitalWrite(PIN_LED_BYPASS, zoom->bypassed);
    }
}
void onFullBypassClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        dprintln(F("BYPASS FULL"));
        zoom->toggleFullBypass();
        digitalWrite(PIN_LED_BYPASS, zoom->bypassed);
    }
}


// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
void setup() {
    dprintinit(9600);

    #ifdef USE_OLED
        display = new OLEDDisplay();
    #elif defined(USE_LCD)
        display = new LCD16x2Display();
    #endif

    // show remote info
    pinMode(PIN_LED_BYPASS, OUTPUT);
    digitalWrite(PIN_LED_BYPASS, LOW);
    display->showRemoteInfo(GIT_TAG, GIT_HASH);
    delay(2000);

    
    // peripheral init
    display->clear();
    display->showString(F("USB INIT..."), 0, 0);
    zoom = new ZoomMSDevice();
    display->showDeviceInfo(zoom->device_name, zoom->fw_version);
    delay(2000);
    
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
    
    _btBypass.attachClick(onBypassClicked);
    
    refreshUI();
}

void loop() {
    _btPrev.tick();
    _btNext.tick();
    _btBypass.tick();
}