
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
#include "class/hid/hid_device.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define UART_TX_PIN 12
#define UART_RX_PIN 13
#define BAUD_RATE 115200

const uint8_t pos[] = { 0, 1, 2, 3, 4 }; // don't touch this

// PIUIO input and output data
static uint8_t inputData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t procInputData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
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

void setup_uart() {
    uart_init(UART_ID, BAUD_RATE); // Configure UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // TX
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // RX
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
//#define PIUIO_BUTTON
#ifndef PIUIO_BUTTON
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
    LXInputData[10] = 0xFF;
    LXInputData[11] = 0xFF;
    LXInputData[14] = 0xFF;
    LXInputData[15] = 0xFF;
#else
    LXInputData[0] = 0xFF;
    LXInputData[1] = 0xFF;
    LXInputData[2] = 0xFF;
    LXInputData[3] = 0xFF;
    LXInputData[4] = 0xFF;
    LXInputData[5] = 0xFF;
    LXInputData[6] = 0xFF;
    LXInputData[7] = 0xFF;
    LXInputData[8] = inputData[1];
    LXInputData[9] = inputData[3];
    LXInputData[10] = inputData[0];
    LXInputData[11] = inputData[2];
    LXInputData[14] = 0xFF;
    LXInputData[15] = 0xFF;
#endif

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
    //memcpy(report, LXInputData, len);
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


int next_device = 0;
static int prev_switch;
static int prev_switch_state;
static int switch_notif = 0;

void go_next_device(void) {
}

static struct lampArray prevLamp = {};
static uint8_t prevInputData [] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static absolute_time_t last_frame_time;
static absolute_time_t last_inactive_demo;
static absolute_time_t now;
static int inactive_index = 0;

const uint8_t inactiveLamps [] = {
    0x01, 0x01,
    0x02, 0x02,
    0x04, 0x04,
    0x08, 0x08,
    0x10, 0x10,
    0x00, 0x00,
    0x02, 0x02,
    0x01, 0x01,
    0x04, 0x04,
    0x10, 0x10,
    0x08, 0x08,
    0x00, 0x00,
    0x1F, 0x1F,
    0x1F, 0x1F,
    0x00, 0x00,
};

void change_dev_task() {
    // This has a debouncer
    volatile uint32_t* a = (uint32_t*)inputData;
    int this_switch = (*a) == 0xFFFEFDFE ? 1 : 0;
    //int this_switch = 0;
    //if(!(inputData[PLAYER_1] & (1 << pos[1]))) this_switch = 0;
    //if(!(inputData[PLAYER_2] & (1 << pos[1]))) this_switch = 0;
    //if(!(inputData[1] & (1 << 1))) this_switch = 0;

    prev_switch = prev_switch << 1;
    prev_switch |= this_switch;
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
}

void piuio_task(void) {
    // P1 / P2 inputs
    memset(procInputData, 0xFF, 8);
    for (int i = 0; i < 5; i++) {
        uint8_t* p1 = &procInputData[PLAYER_1];
        uint8_t* p2 = &procInputData[PLAYER_2];
        if(pinSwitch[i] != 255) *p1 &= ~(((uint8_t) !gpio_get(pinSwitch[i])) << pos[i]); //*p1 =  ? tu_bit_set(*p1, pos[i]) : tu_bit_clear(*p1, pos[i]);
        if(pinSwitch[i+5] != 255) *p2 &= ~(((uint8_t) !gpio_get(pinSwitch[i+5])) << pos[i]); // *p2 = gpio_get(pinSwitch[i+5]) ? tu_bit_set(*p2, pos[i]) : tu_bit_clear(*p2, pos[i]);
    }

    // Test/Service buttons
    // if(pinSwitch[10] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[10])) << 1); // procInputData[1] = gpio_get(pinSwitch[10]) ? tu_bit_set(procInputData[1], 1) : tu_bit_clear(procInputData[1], 1);
    // if(pinSwitch[11] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[11])) << 6); // procInputData[1] = gpio_get(pinSwitch[11]) ? tu_bit_set(procInputData[1], 6) : tu_bit_clear(procInputData[1], 6);
    // if(pinSwitch[12] != 255) procInputData[1] &= ~(((uint8_t) !gpio_get(pinSwitch[12])) << 2); //procInputData[1] = gpio_get(pinSwitch[12]) ? tu_bit_set(procInputData[1], 2) : tu_bit_clear(procInputData[1], 2);
    // if(pinSwitch[13] != 255) procInputData[3] &= ~(((uint8_t) !gpio_get(pinSwitch[13])) << 2); //procInputData[3] = gpio_get(pinSwitch[13]) ? tu_bit_set(procInputData[3], 2) : tu_bit_clear(procInputData[3], 2);

    for(int i =0; i < 8; i++) {
        inputData[i] = procInputData[i];
    }

    // Write pad lamps

    for (int i = 0; i < 5; i++) {
        if(pinLED[i] != 255) gpio_put(pinLED[i], !tu_bit_test(lamp.data[PLAYER_1], pos[i] + 2));
        if(pinLED[i+5] != 255) gpio_put(pinLED[i+5], !tu_bit_test(lamp.data[PLAYER_2], pos[i] + 2));
    }

    // 5. Recebe os dados de Coin, Test e Service via UART0
    if (uart_is_readable(UART_ID)) {
        uint8_t received = uart_getc(UART_ID);

        // Acende LED da Pico A quando receber botão pressionado via UART
        bool any_pressed = false;

        if (!(received & (1 << 1))) {
            inputData[CABINET] &= ~(1 << 1);
            any_pressed = true;
            sleep_ms(2);
        }
        
        if (!(received & (1 << 2))) {
            inputData[CABINET] &= ~(1 << 2);
            any_pressed = true;
            sleep_ms(2);
        }
        
        if (!(received & (1 << 6))) {
            inputData[CABINET] &= ~(1 << 6);
            any_pressed = true;
            sleep_ms(2);
        }

        if (!(received & (1 << 7))) {
            inputData[CABINET] &= ~(1 << 7);
            any_pressed = true;
            sleep_ms(2);
        }

        gpio_put(pinled, any_pressed ? 1 : 0);

    }
}

int main(void) {
    board_init();

    setup_uart();

    // Set up GPIO pins: Inputs first, then outputs
    for (int i = 0; i < (sizeof(pinSwitch)/sizeof(pinSwitch[0])); i++) if(pinSwitch[i] != 255) {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], false);
        gpio_pull_up(pinSwitch[i]);
    }

    prev_switch = 0;
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
        change_dev_task();
        hid_task();
    }

    return 0;
}
