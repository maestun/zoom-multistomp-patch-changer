#include <Arduino.h>
#include "zoom_ms.h"
#include "debug.h"
#include "version.h"
#include "display.h"

#define PIN_BUTTON_NEXT (A1)
#define PIN_BUTTON_PREV (A2)

ZoomMS      _zoom;
IDisplay *  _disp = nullptr;

typedef void (*button_cb_t)(uint8_t);
class Button {
private:
    uint8_t _pin;
    uint16_t _debounce;
    button_cb_t _callback;
    bool _locked;
public:
    Button(uint8_t pin, uint16_t debounce, button_cb_t callback) : 
        _pin(pin), 
        _debounce(debounce), 
        _callback(callback), 
        _locked(false) {

        pinMode(pin, INPUT_PULLUP);
    }

    void tick() {
        if (digitalRead(_pin)) {
            if (_locked == false) {
                _locked = true;
                _callback(_pin);
                delay(_debounce);
            }
        }
        else {
            _locked = false;
        }
    }
};


void refreshUI() {
    uint8_t index = _zoom.request_patch_index();
    char * name = _zoom.get_preloaded_name(index);
    _disp->showPatch(index, name);
}


void on_patch_name_preloaded(uint8_t index, char* name) {
    dprint(index + 1);
    dprint(F("/50 - "));
    dprintln(name);
    
    char str[6] = "";
    sprintf(str, "%d %%", (int)((int)index * 100) / (ZOOM_MS_MAX_PATCHES - 1));
    _disp->showString(str, 0, 1);
}


void on_next(uint8_t dummy) {   
    uint8_t index = _zoom.next_patch();
    _disp->showPatch(index, _zoom.get_preloaded_name(index));
    dprintln(F("next"));
}


void on_prev(uint8_t dummy) {
    uint8_t index = _zoom.prev_patch();
    _disp->showPatch(index, _zoom.get_preloaded_name(index));
    dprintln(F("prev"));
}

Button _next(PIN_BUTTON_NEXT, 50, &on_next);
Button _prev(PIN_BUTTON_PREV, 50, &on_prev);

void setup() {
    dprintinit(9600);

    _disp = display_instance();
    _disp->showRemoteInfo(GIT_TAG, GIT_HASH);
    delay(1000);

    _zoom.connect();
    _disp->clear();
    _disp->showString(_zoom.device_name, 0, 0);
    _disp->showString(_zoom.fw_version, 0, 1);
    delay(1000);
    
    _disp->clear();
    _disp->showString(F("Preloading..."), 0, 0);
    _zoom.preload_patch_names(&on_patch_name_preloaded);

    _disp->clear();
    refreshUI();
}


void loop() {

#ifdef SERIAL_DEBUG
    int c = Serial.read();
    if (c == 'n') {
        on_next(0);
    }
    else if (c == 'p') {
        on_prev(0);
    }
#endif
    _next.tick();
    _prev.tick();
    delay(50);
}
