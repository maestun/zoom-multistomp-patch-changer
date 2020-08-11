// -------------------------------------------------------------------------------
// Zoom MultiStomp patch selector
// 
// Increment / decrement patch numbers (up to 50) using your feet.
// A longpress will trigger the cycling through the patches.
// The last selected patch will be saved into Arduino's EEPROM, 
// and called back on startup.
//
// Libraries used:
// - USB Host Shield Library v2.0 ( https://github.com/felis/USB_Host_Shield_2.0 )
// - Adafruit_SSD1306 ( https://github.com/adafruit/Adafruit_SSD1306 )
// -------------------------------------------------------------------------------
#include "debug.h"
#include "zoomMS.h"

// Footswitch pins for patch increment / decrement.
// Vcc ---> Button pin 1
//          Button pin 2 ---> Arduino
//                       ---> 10k resistor ---> GND
#define PIN_BUTTON_PREV             (4)
#define PIN_BUTTON_NEXT             (5)

// Delay (in milliseconds) for a 'longpress' event to happen.
#define LONGPRESS_THRESHOLD_MS      (1000)

// Delay (in milliseconds) between two patch increments.
#define AUTOCYCLE_DELAY_MS          (500)


ZoomMS * gZoom;


void setup() {
    dprintinit(9600);
    gZoom = new ZoomMS(PIN_BUTTON_PREV, PIN_BUTTON_NEXT, LONGPRESS_THRESHOLD_MS, AUTOCYCLE_DELAY_MS);
}


void loop() {
    gZoom->scan();
    delay(5);
}
