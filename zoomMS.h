#include "debug.h"
#include "button.h"
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Usb.h>
#include <usbh_midi.h>


class ZoomMS : public ButtonListener {
private:
    int                 _currentPatch;
    uint8_t             _prevPin;
    uint8_t             _deviceID;
    uint8_t             _nextPin;
    uint32_t            _cycleTS;
    uint16_t            _cycleMS;
    Button *            _prevButton;
    Button *            _nextButton;
    Adafruit_SSD1306 *  _display;
    
    void                onButtonEvent(uint8_t aPin, EButtonScanResult aResult);
    void                initDisplay();
    void                initUSB();
    void                updateDisplay();
    void                updatePatch(int inc);
    void                sendPatch();
    void                loadPatch();
    void                savePatch();
public:
    ZoomMS(uint8_t aPrevPin, uint8_t aNextPin, uint16_t aLongpressDelay, uint16_t aAutoCycleDelayMS);
    void scan();
};
