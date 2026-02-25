
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

uint8_t all_states[4][2]; // [mux][player]

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
    return tud_vendor_control_xfer_cb_lxio(rhport, stage, request);
}

void handle_lxio_data()
{
    // NÃO sobrescrever LXInputData aqui
    // Apenas atualizar os lamps vindos do jogo

    memcpy(lamp.data, LXLampData, 8);
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
    // Variável para guardar o último envio
    static uint32_t last_time_us = 0;
    
    // Limita o envio para não travar a porta USB (1000 microsegundos = 1ms)
    if (time_us_32() - last_time_us < 1000) return;

    if ( tud_hid_n_ready(0) )
    {
        tud_hid_n_report(0, 0, LXInputData, 16);
        last_time_us = time_us_32();
    }
}

void piuio_task(void)
{
    // --------------------------------------------------
    // 1. Ler os 4 estados do mux
    // --------------------------------------------------

// Ler o canal do multiplexador solicitado pelo jogo
uint8_t current_mux1 = LXLampData[0] & 0x03;
uint8_t current_mux2 = LXLampData[2] & 0x03;

// Selecionar o mux para ambos os players
gpio_put(pinNXP1[0], current_mux1 & 1);
gpio_put(pinNXP1[1], (current_mux1 >> 1) & 1);

gpio_put(pinNXP2[0], current_mux2 & 1);
gpio_put(pinNXP2[1], (current_mux2 >> 1) & 1);

sleep_us(20);

uint8_t stateP1 = 0xFF;
uint8_t stateP2 = 0xFF;

for (int btn = 0; btn < 5; btn++) {
    if (!gpio_get(pinSwitch[btn]))
        stateP1 &= ~(1 << pos[btn]);

    if (!gpio_get(pinSwitch[btn + 5]))
        stateP2 &= ~(1 << pos[btn]);
}

// Escrever diretamente no buffer correto para os dois players
LXInputData[current_mux1] = stateP1;
LXInputData[4 + current_mux2] = stateP2;

}

    // 5. Recebe os dados de Coin, CoinClear, Test e Service via UART0
    // static uint8_t last_received = 0xFF;

    // if (uart_is_readable(UART_ID)) {
    //     last_received = uart_getc(UART_ID);
    // }

    // uint8_t cabinet_state = 0xFF;

    // if (!(last_received & (1 << 1))) 
    //     cabinet_state &= ~(1 << 1); // Service button
    
    // if (!(last_received & (1 << 2))) 
    //     cabinet_state &= ~(1 << 2); // Test button

    // if (!(last_received & (1 << 6))) 
    //     cabinet_state &= ~(1 << 6); // CoinClear input

    // if (!(last_received & (1 << 7))) 
    //     cabinet_state &= ~(1 << 7); // Coin input

    // inputData[CABINET] = cabinet_state;
    
    // gpio_put(25, cabinet_state != 0xFF);



int main(void) {
    board_init();

    // setup_uart();

    // Set up GPIO pins: Inputs first, then outputs
    for (int i = 0; i < 10; i++) {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], false);
        gpio_pull_up(pinSwitch[i]);
    }

    for (int i = 0; i < 10; i++) {
        gpio_init(pinLED[i]);
        gpio_set_dir(pinLED[i], true);
    }

    gpio_init(pinled);
    gpio_set_dir(pinled, true);

    
    // MUX PLAYER 1
    gpio_init(pinNXP1[0]);
    gpio_set_dir(pinNXP1[0], GPIO_OUT);

    gpio_init(pinNXP1[1]);
    gpio_set_dir(pinNXP1[1], GPIO_OUT);

    // MUX PLAYER 2
    gpio_init(pinNXP2[0]);
    gpio_set_dir(pinNXP2[0], GPIO_OUT);

    gpio_init(pinNXP2[1]);
    gpio_set_dir(pinNXP2[1], GPIO_OUT);

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
