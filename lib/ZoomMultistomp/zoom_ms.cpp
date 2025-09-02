#include <Usb.h>
#include <usbh_midi.h>
#include "zoom_ms_debug.h"
#include "zoom_ms.h"

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
#define DEV_MAX_FX_PER_PATCH        (5)

USB                                 _usb;
uint8_t 	                        _readBuffer[MIDI_MAX_SYSEX_SIZE] = {0};
USBH_MIDI                           _midi(&_usb);
uint8_t                             _patch_len = 0;


// preloaded patch names
char                PATCH_NAMES[ZOOM_MS_MAX_PATCHES * (ZOOM_MS_PATCH_NAME_LEN + 1)] = {0};

// id request
uint8_t 			ID_PAK[] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };
// set editor mode on / off
uint8_t 			EM_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x50 /* 0x50: on - 0x51: off */, 0xf7 };
// get patch index
uint8_t 			PI_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x33, 0xf7 };
// get patch data
uint8_t 			PD_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x29, 0xf7};
// set tuner mode on / off
uint8_t 			TU_PAK[] = { 0xb0, 0x4a, 0x00 /* 0x41: on - 0x0: off */ };
// program change
uint8_t 			PC_PAK[] = { 0xc0, 0x00 /* program number */ };
// bypass effect
uint8_t 			BP_PAK[] = { 0xf0, 0x52, 0x00, 0xff /* device ID */, 0x31, 0x00 /* effect slot number */, 0x00, 0x00 /* 1: on, 0: off */, 0x00, 0x00, 0xf7 };


void ZoomMS::usb_task() {
    int state = 0; 
    do {
        _usb.Task();
        state = _usb.getUsbTaskState();
    } while(state != USB_STATE_RUNNING);
}


void ZoomMS::send_bytes(uint8_t * aBytes) {
    usb_task();
    _midi.SendData(aBytes);
}


void ZoomMS::read_usb_response() {
    uint16_t recv_read = 0;
    uint16_t recv_count = 0;
    uint8_t rcode = 0;
    usb_task();

    do {
        delay(10);
        rcode = _midi.RecvData(&recv_read, (uint8_t *)(_readBuffer + recv_count));
        if(rcode == 0) {
            recv_count += recv_read;
        }
    } while (rcode == 0);

    for (uint16_t i = 0, j = 0; i < recv_count; i++) {
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
        _readBuffer[j++] = _readBuffer[++i];
    }  
}


void ZoomMS::request_device_id() {
    send_bytes(ID_PAK);
    read_usb_response();
    uint8_t deviceID = _readBuffer[6];

    BP_PAK[3] = deviceID;
    EM_PAK[3] = deviceID;
    PI_PAK[3] = deviceID;
    PD_PAK[3] = deviceID;

    fw_version[0] = _readBuffer[10];
    fw_version[1] = _readBuffer[11];
    fw_version[2] = _readBuffer[12];
    fw_version[3] = _readBuffer[13];

    device_name = DEV_NAME_INVALID;
    switch(deviceID) {
    case DEV_ID_MS_50G:
		_patch_len = DEV_PLEN_MS_50G;
		device_name = DEV_NAME_MS_50G;
		break;
    case DEV_ID_MS_70CDR:
		_patch_len = DEV_PLEN_MS_70CDR;
		device_name = DEV_NAME_MS_70CDR;
		break;
    case DEV_ID_MS_60B:
		_patch_len = DEV_PLEN_MS_60B;
		device_name = DEV_NAME_MS_60G;
		break;
    }

    dprint(F("DEVICE NAME: "));
    dprintln(device_name);

    dprint(F("DEVICE FW: "));
    dprintln(fw_version);

    dprint(F("PATCH LEN: "));
    dprintln(_patch_len);
}


void ZoomMS::enable_editor_mode(bool aEnable) {
	EM_PAK[4] = aEnable ? 0x50 : 0x51;
    send_bytes(EM_PAK);
}


void ZoomMS::send_patch(uint8_t patch_index) {
    if (patch_index < ZOOM_MS_MAX_PATCHES) {
        PC_PAK[1] = patch_index;
        send_bytes(PC_PAK);
        dprint(F("Send patch: "));
        dprintln(patch_index + 1);
    }
}


uint8_t ZoomMS::request_patch_index() {
    send_bytes(PI_PAK);
    read_usb_response();
    uint8_t index = _readBuffer[7];
    dprint(F("Current patch: "));
    dprintln(index);
    return index;
}


char* ZoomMS::request_patch_name() {
    
    send_bytes(PD_PAK);
    read_usb_response();

    static char patch_name[ZOOM_MS_PATCH_NAME_LEN + 1];
    patch_name[0] = _readBuffer[_patch_len - 14];
    patch_name[1] = _readBuffer[_patch_len - 12];
    patch_name[2] = _readBuffer[_patch_len - 11];
    patch_name[3] = _readBuffer[_patch_len - 10];
    patch_name[4] = _readBuffer[_patch_len - 9];
    patch_name[5] = _readBuffer[_patch_len - 8];
    patch_name[6] = _readBuffer[_patch_len - 7];
    patch_name[7] = _readBuffer[_patch_len - 6];
    patch_name[8] = _readBuffer[_patch_len - 4];
    patch_name[9] = _readBuffer[_patch_len - 3];
    patch_name[10] = '\0';
    return patch_name;
}


char * ZoomMS::get_preloaded_name(uint8_t patch_index) {
    return (PATCH_NAMES + (patch_index * (ZOOM_MS_PATCH_NAME_LEN + 1)));
}


void ZoomMS::preload_patch_names(preload_fptr fptr) {

    // save current patch
    uint8_t pi = request_patch_index();

    // loop thru all patches
    const int MIN_DELAY_FOR_DATA_REQ = 200;
    for (int patch_idx = 0; patch_idx < ZOOM_MS_MAX_PATCHES; patch_idx++) {
        send_patch(patch_idx);
        delay(MIN_DELAY_FOR_DATA_REQ);
        char * name = request_patch_name();
        fptr(patch_idx, name);
        memcpy(PATCH_NAMES + (patch_idx * (ZOOM_MS_PATCH_NAME_LEN + 1)), name, ZOOM_MS_PATCH_NAME_LEN + 1);
        dprintln(PATCH_NAMES + (patch_idx * (ZOOM_MS_PATCH_NAME_LEN + 1)));
    }

    // restore prev patch
    send_patch(pi);
    delay(MIN_DELAY_FOR_DATA_REQ);
    dprintln(F("preload ok"));
}


void ZoomMS::connect() {
    _usb.Init();
    request_device_id();
    enable_editor_mode(true);
}


uint8_t ZoomMS::next_patch() {
    uint8_t index = request_patch_index();
    if (++index == ZOOM_MS_MAX_PATCHES) {
        index = 0;
    }
    send_patch(index);
    return index;
}


uint8_t ZoomMS::prev_patch() {
    int8_t index = request_patch_index();
    if (--index < 0) {
        index = ZOOM_MS_MAX_PATCHES - 1;
    }
    send_patch(index);
    return index;
}


ZoomMS::ZoomMS() {
    dprintinit(ZOOM_MS_SERIAL_BAUDS);
}