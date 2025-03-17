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
#  include "display_lcd.h"
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
#define AUTOCYCLE_DELAY_MS          (250)

// ----------------------------------------------------------------------------
// GLOBALS
// ----------------------------------------------------------------------------

// button stuff
uint32_t  			_cycleTS;
uint16_t  			_cycleMS = 200;
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

void onNextClicked();
void onPrevClicked();
void onBypassClicked();
void onNextLongHold();
void onPrevLongHold();
void onNextLongStart();
void onPrevLongStart();
void onNextLongStop();
void onPrevLongStop();


// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
void setup() {
    dprintinit(9600);

    #ifdef USE_OLED
        display = new OLEDDisplay();
    #elif defined(USE_LCD)
        display = new LCDDisplay();
    #endif

    pinMode(PIN_LED_BYPASS, OUTPUT);
    digitalWrite(PIN_LED_BYPASS, LOW);

    display->clear();
    display->showString(F(" ZOOM MS REMOTE "), 0, 0);
    display->showString(GIT_TAG, 0, 1); // oled y=16 / TODO: get_line2() for IDisplay
    display->showString(GIT_HASH, 7, 1);// oled y=16 / TODO: get_line2() for IDisplay
    delay(1000);

    // peripheral init
    display->showString(F("    USB INIT    "), 0, 0);
    zoom = new ZoomMSDevice();   
    // display->clear();
    display->showString(zoom->device_name, 0, 1); // oled y=16 / TODO: get_line2() for IDisplay
    display->showString(zoom->fw_version, 9, 1); // oled y=16 / TODO: get_line2() for IDisplay
    delay(1000);

    display->showPatch(zoom->patch_index, zoom->patch_name);

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
}


void loop() {
    _btPrev.tick();
    _btNext.tick();
    _btBypass.tick();
}

  
void toggleTuner() {
	if(zoom->tuner_enabled) {
        display->showString(F(" TUNER ON "), 0, 0);
	}
    else {
        display->showPatch(zoom->patch_index, zoom->patch_name);
    	// this flag will prevent patch scrolling 
    	// if buttons are released not quite simultaneously
    	_cancelScroll = true;
    }
}


// ----------------------------------------------------------------------------
// BUTTON CALLBACKS
// ----------------------------------------------------------------------------

// ============================================================================
// Toggle tuner only if:
// - we're not scrolling through the patches
// - both buttons are being longpressed
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


// ============================================================================
// Scroll through patches only if:
// - tuner is disabled AND
// - the other button is not currently pressed
void onNextLongHold() {
  if(zoom->tuner_enabled == false && 
  	 _cancelScroll == false &&
     _btPrevDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= _cycleMS) {
            _cycleTS = ts;
            // Serial.println("SCROLL NEXT...");
            zoom->incPatch(1);
        }
    }
} 
void onPrevLongHold() {
    if(zoom->tuner_enabled == false && 
       _cancelScroll == false &&
       _btNextDown == false) {

        _isScrolling = true;
        uint32_t ts = millis();
        if((ts - _cycleTS) >= _cycleMS) {
            _cycleTS = ts;
            // Serial.println("SCROLL PREV...");
            zoom->incPatch(-1);
        }
    }
}


// ============================================================================
// Reset button state
void onNextLongStop() {
    _btNextDown = false;
    _isScrolling = false;
}
void onPrevLongStop() {
    _btPrevDown = false;
    _isScrolling = false;
}


// ============================================================================
// Single patch increment only if: 
// - tuner is disabled AND
// - we're not scrolling with the other button
void onNextClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        zoom->incPatch(1);
    }
    _btNextDown = false;
}
void onPrevClicked() {
    if(zoom->tuner_enabled == false && !_isScrolling) {
        zoom->incPatch(-1);
    }
    _btPrevDown = false;
}


// ============================================================================
// Toggle bypass only if: 
// - tuner is disabled AND
// - we're not scrolling
void onBypassClicked() {
    dprintln(F("BYPASS"));
    if(zoom->tuner_enabled == false && !_isScrolling) {
        zoom->toggleBypass();
        digitalWrite(PIN_LED_BYPASS, !zoom->bypassed);
        // digitalWrite(PIN_LED_BYPASS, zoom->bypassed);
    }
}
// void onFullBypassClicked() {
//     dprintln(F("BYPASS FULL"));
//     if(_tunerEnabled == false && !_isScrolling) {
//         toggleFullBypass();
//     }
// }