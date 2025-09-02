#pragma once

#include <stdint.h>

#define ZOOM_MS_MAX_PATCHES         (50)
#define ZOOM_MS_PATCH_NAME_LEN      (10)

typedef void (*preload_fptr)(uint8_t, char *);

class ZoomMS {

private:
    void usb_task();
    void send_bytes(uint8_t * bytes);
    void read_usb_response();

public:
    ZoomMS();

    // communication routines
    void preload_patch_names(preload_fptr fptr);
    void connect();
    void enable_editor_mode(bool enable);
    void send_patch(uint8_t patch_index);
    uint8_t request_patch_index();
    char * request_patch_name();
    void request_device_id();
    uint8_t next_patch();
    uint8_t prev_patch();

    // returns a pointer to a preloaded patch name
    char * get_preloaded_name(uint8_t patch_index);

public:
    const __FlashStringHelper * device_name;
    char                        fw_version[5] = {0};

};