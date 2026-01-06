
#include "bsp/board.h"
#include "device/usbd.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "true_lxio_structs.h"
#include "true_lxio_config.h"
#include "usb_descriptors.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "class/hid/hid.h"

#ifdef ENABLE_WS2812_SUPPORT
#include "true_lxio_ws2812.h"
#endif

#define FLASH_TARGET_OFFSET (512 * 1024) // choosing to start at 512K

// From https://forums.raspberrypi.com/viewtopic.php?t=310821
struct MyData {
    int which;
};

struct MyData myData;

void saveMyData() {
    uint8_t* myDataAsBytes = (uint8_t*) &myData;
    int myDataSize = sizeof(myData);
    
    int writeSize = (myDataSize / FLASH_PAGE_SIZE) + 1; // how many flash pages we're gonna need to write
    int sectorCount = ((writeSize * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE) + 1; // how many flash sectors we're gonna need to erase
        
    printf("Programming flash target region...\n");

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE * sectorCount);
    flash_range_program(FLASH_TARGET_OFFSET, myDataAsBytes, FLASH_PAGE_SIZE * writeSize);
    restore_interrupts(interrupts);

    printf("Done.\n");
}

void readBackMyData() {
    const uint8_t* flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(&myData, flash_target_contents, sizeof(myData));
}


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
    memcpy(report, LXInputData, len);
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

void hid_task(void) {
  if(piuio_which_device == 0) return; // Dont do this on PIUIO
  // Just do the sendings

  if ( tud_hid_n_ready(0) )
  {
    handle_lxio_data();
    tud_hid_n_report(0, 0, LXInputData, 16);
  }
}


static int next_device = 0;
static int prev_switch;
static int prev_switch_state;
static int switch_notif = 0;

void go_next_device(void) {
    next_device++;
    if(next_device >= 3) next_device = 0;
    myData.which = next_device;
    saveMyData();
}

void piuio_task(void) {
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_lock_mtx();
    #endif

    // P1 / P2 inputs
    for (int i = 0; i < 5; i++) {
        uint8_t* p1 = &inputData[PLAYER_1];
        uint8_t* p2 = &inputData[PLAYER_2];
        if(pinSwitch[i] != 255) *p1 = gpio_get(pinSwitch[i]) ? tu_bit_set(*p1, pos[i]) : tu_bit_clear(*p1, pos[i]);
        if(pinSwitch[i+5] != 255) *p2 = gpio_get(pinSwitch[i+5]) ? tu_bit_set(*p2, pos[i]) : tu_bit_clear(*p2, pos[i]);
    }

    // Test/Service buttons
    if(pinSwitch[10] != 255) inputData[CABINET] = gpio_get(pinSwitch[10]) ? tu_bit_set(inputData[1], 1) : tu_bit_clear(inputData[1], 1);
    if(pinSwitch[11] != 255) inputData[CABINET] = gpio_get(pinSwitch[11]) ? tu_bit_set(inputData[1], 6) : tu_bit_clear(inputData[1], 6);
    if(pinSwitch[12] != 255) inputData[CABINET] = gpio_get(pinSwitch[12]) ? tu_bit_set(inputData[1], 2) : tu_bit_clear(inputData[1], 2);
    if(pinSwitch[13] != 255) inputData[CABINET] = gpio_get(pinSwitch[13]) ? tu_bit_set(inputData[3], 2) : tu_bit_clear(inputData[3], 2);

    // Write pad lamps
    for (int i = 0; i < 5; i++) {
        if(pinLED[i] != 255) gpio_put(pinLED[i], tu_bit_test(lamp.data[PLAYER_1], pos[i] + 2));
        if(pinLED[i+5] != 255) gpio_put(pinLED[i+5], tu_bit_test(lamp.data[PLAYER_2], pos[i] + 2));
    }

    // This has a debouncer
    prev_switch = prev_switch << 1;
    prev_switch |= gpio_get(pinswitchlxio)?1:0;
    prev_switch &= 0xF;
    int cur_switch_state = prev_switch_state;
    if(prev_switch == 0xF) cur_switch_state = 1;
    else if(prev_switch == 0x0) cur_switch_state = 0;
    if(prev_switch_state != cur_switch_state && !cur_switch_state) {
        go_next_device();
        switch_notif = 1;
    }
    if(prev_switch_state != cur_switch_state && cur_switch_state) {
        switch_notif = 0;
    }
    prev_switch_state = cur_switch_state;

    // Write the bass neon to the onboard LED for testing + kicks
    gpio_put(pinled, lamp.counter2 | switch_notif);

    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_unlock_mtx();
    #endif
}

int main(void) {
    board_init();
    readBackMyData();

    piuio_which_device = (int)myData.which;
    if(piuio_which_device < 0 || piuio_which_device > 2) piuio_which_device = 1;
    next_device = piuio_which_device;

    // Init WS2812B
    #ifdef ENABLE_WS2812_SUPPORT
    ws2812_init(&lamp);
    #endif

    // Set up GPIO pins: Inputs first, then outputs
    for (int i = 0; i < (sizeof(pinSwitch)/sizeof(pinSwitch[0])); i++) if(pinSwitch[i] != 255) {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], false);
        gpio_pull_up(pinSwitch[i]);
    }

    gpio_init(pinswitchlxio);
    gpio_set_dir(pinswitchlxio, false);
    gpio_pull_up(pinswitchlxio);

    prev_switch = gpio_get(pinswitchlxio)?1:0;
    for(int i = 1; i < 4; i++) {
        prev_switch |= (prev_switch & 1) << i;
    }
    prev_switch_state = prev_switch == 0xF?1:0;

    for (int i = 0; i < (sizeof(pinLED)/sizeof(pinLED[0])); i++) if(pinLED[i] != 255) {
        gpio_init(pinLED[i]);
        gpio_set_dir(pinLED[i], true);
    }

    gpio_init(pinled);
    gpio_set_dir(pinled, true);

    // init device stack on configured roothub port
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
    };
    tud_rhport_init(BOARD_TUD_RHPORT, &rh_init);

    // Main loop
    while (true) {
        tud_task(); // tinyusb device task
        piuio_task();
        hid_task();
    }

    return 0;
}
