
#include "bsp/board.h"
#include "device/usbd.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "true_lxio_structs.h"
#include "true_lxio_config.h"
#include "usb_descriptors.h"

#ifdef ENABLE_WS2812_SUPPORT
#include "true_lxio_ws2812.h"
#endif

const uint8_t pos[] = { 3, 0, 2, 1, 4 }; // don't touch this

// PIUIO input and output data
uint8_t inputData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
struct lampArray lamp = {};

uint8_t LXInputData[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t LXLampData[16] = {};

bool tud_vendor_control_xfer_cb_piuio(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // Request 0xAE = IO Time
    if (request->bRequest == 0xAE) {
        switch (request->bmRequestType) {
            case 0x40:
                return tud_control_xfer(rhport, request, (void *)&lamp.data, 8);
            case 0xC0:
                return tud_control_xfer(rhport, request, (void *)&inputData, 8);
            default:
                return false;
        }
    }

    return false;
}

bool tud_vendor_control_xfer_cb_lxio(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // Control should be handled by the HID at this point
    return false;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    if (piuio_which_device == 0) 
        return tud_vendor_control_xfer_cb_piuio(rhport, stage, request);
    
    else
        return tud_vendor_control_xfer_cb_lxio(rhport, stage, request);
}

void handle_lxio_data() {
    // Do the conversion from the Lights and Input from regular IO
    // to the LX Lights and Input format
    // NOTE: Note that InputData will carry the 4 sensor anyways
    LXInputData[0] = inputData[0];
    LXInputData[1] = inputData[0];
    LXInputData[2] = inputData[0];
    LXInputData[3] = inputData[0];
    LXInputData[4] = inputData[2];
    LXInputData[5] = inputData[2];
    LXInputData[6] = inputData[2];
    LXInputData[7] = inputData[2];
    LXInputData[8] = inputData[1];
    LXInputData[9] = inputData[3];
    LXInputData[10] = 0xFF; // TODO: We do not have a handler for the front buttons
    LXInputData[11] = 0xFF; // TODO: We do not have a handler for the front buttons
    LXInputData[14] = 0xFF;
    LXInputData[15] = 0xFF;

    memcpy(lamp.data, LXLampData, 8); // Just copy in the case of lamps
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void) itf;
    (void) buffer;
    (void) bufsize;
    // Not implemented, but maybe is necessary
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void) itf;
    (void) sent_bytes;
    // Not implemented, but maybe is necessary
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;

    // Get the LX data
    len = len > 16 ? 16 : len;
    memcpy(LXLampData, report, len);
    handle_lxio_data();
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) itf;
    (void) report_id;
    (void) report_type;

    // Get the LX data
    bufsize = bufsize > 16 ? 16 : bufsize;
    memcpy(LXLampData, buffer, bufsize);
    handle_lxio_data();
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    // Put the LX data
    handle_lxio_data();
    reqlen = reqlen > 16 ? 16 : reqlen;
    memcpy(buffer, LXInputData, reqlen);
    return reqlen;
}

void piuio_task(void) {
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_lock_mtx();
    #endif

    // P1 / P2 inputs
    for (int i = 0; i < 5; i++) {
        uint8_t* p1 = &inputData[PLAYER_1];
        uint8_t* p2 = &inputData[PLAYER_2];
        *p1 = gpio_get(pinSwitch[i]) ? tu_bit_set(*p1, pos[i]) : tu_bit_clear(*p1, pos[i]);
        *p2 = gpio_get(pinSwitch[i+5]) ? tu_bit_set(*p2, pos[i]) : tu_bit_clear(*p2, pos[i]);
    }

    // Test/Service buttons
    inputData[CABINET] = gpio_get(pinSwitch[10]) ? tu_bit_set(inputData[1], 1) : tu_bit_clear(inputData[1], 1);
    inputData[CABINET] = gpio_get(pinSwitch[11]) ? tu_bit_set(inputData[1], 6) : tu_bit_clear(inputData[1], 6);

    // Write pad lamps
    for (int i = 0; i < 5; i++) {
        gpio_put(pinLED[i], tu_bit_test(lamp.data[PLAYER_1], pos[i] + 2));
        gpio_put(pinLED[i+5], tu_bit_test(lamp.data[PLAYER_2], pos[i] + 2));
    }

    // Write the bass neon to the onboard LED for testing + kicks
    gpio_put(25, lamp.bass_light);

    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_unlock_mtx();
    #endif
}

int main(void) {
    board_init();

    // Init WS2812B
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_init(&lamp);
    #endif

    // Set up GPIO pins: Inputs first, then outputs
    for (int i = 0; i < 12; i++) {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], false);
        gpio_pull_up(pinSwitch[i]);
    }
    
    gpio_init(pinswitchlxio);
    gpio_set_dir(pinswitchlxio, false);
    gpio_pull_up(pinswitchlxio);

    if(gpio_get(pinswitchlxio)) {
        piuio_which_device = 0; // Switch back to PIUIO if pressed
    }

    for (int i = 0; i < 10; i++) {
        gpio_init(pinLED[i]);
        gpio_set_dir(pinLED[i], true);
    }

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    // Main loop
    while (true) {
        tud_task(); // tinyusb device task
        piuio_task();
    }

    return 0;
}
