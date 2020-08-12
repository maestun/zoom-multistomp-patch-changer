#include "debug.h"
#include "button.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Usb.h>
#include <usbh_midi.h>


class ZoomMS : public ButtonListener {
private:
    // device stuff
    int                 _currentPatch;
    char                _currentPatchName[11];
    int                 _patchLen;
    uint8_t             _deviceID;
    bool                _tuner;

    // button stuff
    uint8_t             _prevPin;
    uint8_t             _nextPin;
    bool                _prevHeld;
    bool                _nextHeld;
    Button *            _prevButton;
    Button *            _nextButton;

    // timing stuff
    uint32_t            _cycleTS;
    uint16_t            _cycleMS;

    // display stuff
    Adafruit_SSD1306 *  _display;
    
    // callback
    void                onButtonEvent(uint8_t aPin, EButtonScanResult aResult);
    
    // display stuff
    void                initDisplay();
    void                updateDisplay();
    
    // button stuff
    void                incPatch(bool aIsPrev);
    
    // device stuff
    void                initDevice();
    // void                setEditorMode(bool aEnable);
    void                requestPatchData();
    // void                requestPatchIndex();
    void                readResponse();
    void                sendBytes(uint8_t * aBytes, char * aMessage);
    void                sendPatch();
    void                sendTuner(bool aEnable);

public:
    ZoomMS(uint8_t aPrevPin, uint8_t aNextPin, uint16_t aLongpressDelay, uint16_t aAutoCycleDelayMS);
    void scan();
};
