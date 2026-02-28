#include "bsp/board.h"
#include "tusb.h"
#include "class/hid/hid_device.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "true_lxio_structs.h"
#include "true_lxio_config.h"

// -------------------------------------------------------------------
// UART configuration
// -------------------------------------------------------------------

// Use only if you want two RP2040 in communication.
#define UART_ID uart0
#define UART_TX_PIN 12
#define UART_RX_PIN 13
#define BAUD_RATE 115200

void setup_uart() {
    uart_init(UART_ID, BAUD_RATE); // Configure UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // TX
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // RX
}

//--------------------------------------------------------------------
// LXIO buffers
//--------------------------------------------------------------------
#define LX_INPUT_DATA_SIZE 16

uint8_t LXInputData[LX_INPUT_DATA_SIZE] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

struct lampArray lamp = {};

uint8_t LXLampData[16] = {};


//--------------------------------------------------------------------
// Convert IO → LXIO format
//--------------------------------------------------------------------

void handle_lxio_data(void)
{
    memcpy(lamp.data, LXLampData, 8); // Just copy in the case of lamps
}


//--------------------------------------------------------------------
// HID callbacks
//--------------------------------------------------------------------
void tud_hid_set_report_cb(
    uint8_t itf,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const* buffer,
    uint16_t bufsize)
{
    (void)itf;
    (void)report_id;
    (void)report_type;

    if (bufsize > LX_INPUT_DATA_SIZE)
        bufsize = LX_INPUT_DATA_SIZE;

    memcpy(LXLampData, buffer, bufsize);
}


uint16_t tud_hid_get_report_cb(
    uint8_t itf,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t* buffer,
    uint16_t reqlen)
{
    (void)itf;
    (void)report_id;
    (void)report_type;

    handle_lxio_data();

    if (reqlen > LX_INPUT_DATA_SIZE)
        reqlen = LX_INPUT_DATA_SIZE;

    memcpy(buffer, LXInputData, reqlen);

    return reqlen;
}


//--------------------------------------------------------------------
// HID send task
//--------------------------------------------------------------------

void hid_task(void)
{
    if (!tud_hid_ready()) return;

    handle_lxio_data();

    tud_hid_report(0, LXInputData, LX_INPUT_DATA_SIZE);
}

//--------------------------------------------------------------------
// Read physical inputs
//--------------------------------------------------------------------
uint8_t mux_pos = 0; // 0-3, corresponds to the 4 mux positions on the original PCB
uint8_t all_states[4][1]; // mux position, player state. Only 1 byte per player since the original PCB only has 8 inputs per player, but this can be expanded if needed.
uint8_t all_states_p2[4][1]; // same as above but for player 2, since the original PCB has a separate mux for player 2

void piuio_task(void)
{
    // set mux position
    gpio_put(pinNXP1[1], mux_pos & 1);
    gpio_put(pinNXP1[0], (mux_pos >> 1) & 1);

    // duplicate for player 2 mux since original PCB has separate muxes for player 1 and player 2
    gpio_put(pinNXP2[1], mux_pos & 1);
    gpio_put(pinNXP2[0], (mux_pos >> 1) & 1);

    // small delay to allow mux to switch (polling rate 1000Hz, so 1ms delay should be fine)
    sleep_ms(1);

    // read inputs for current mux position and player states
    uint8_t stateP1 = 0xFF;
    uint8_t stateP2 = 0xFF;

    // Loop through the 5 inputs for player 1 and player 2. If pressed, set the corresponding bit to 0 (active low)
    // and save the state in the all_states buffer according to the current mux position.
    // This way we can keep track of the state of all 4 mux positions for both players, even though we're only reading one at a time.
    for (uint8_t i = 0; i < INPUTS_PER_PLAYER; i++)
    {
        bool pressed = !gpio_get(pinSwitch[i]);
        if (pressed) {
            if (mux_pos == 0) stateP1 &= ~(1 << i);
            else if (mux_pos == 1) stateP1 &= ~(1 << i);
            else if (mux_pos == 2) stateP1 &= ~(1 << i);
            else if (mux_pos == 3) stateP1 &= ~(1 << i);
        }
        pressed = !gpio_get(pinSwitch[i+5]);
        if (pressed) {
            if (mux_pos == 0) stateP2 &= ~(1 << i);
            else if (mux_pos == 1) stateP2 &= ~(1 << i);
            else if (mux_pos == 2) stateP2 &= ~(1 << i);
            else if (mux_pos == 3) stateP2 &= ~(1 << i);
        }
    }

    // save the state in the all_states buffer according to the current mux position, and then update the LXInputData buffer with the current state of all mux positions for both players.
    all_states[mux_pos][PLAYER_1] = stateP1;
    LXInputData[0] = all_states[0][PLAYER_1];
    LXInputData[1] = all_states[1][PLAYER_1];
    LXInputData[2] = all_states[2][PLAYER_1];
    LXInputData[3] = all_states[3][PLAYER_1];
    // do the same for player 2
    all_states_p2[mux_pos][PLAYER_2] = stateP2;
    LXInputData[4] = all_states_p2[0][PLAYER_2];
    LXInputData[5] = all_states_p2[1][PLAYER_2];
    LXInputData[6] = all_states_p2[2][PLAYER_2];
    LXInputData[7] = all_states_p2[3][PLAYER_2];

    // increment mux position for next loop
    mux_pos++;
    // wrap around mux position if it exceeds 3 (since we only have 4 mux positions: 0, 1, 2, 3)
    mux_pos &= 3;

    // Write pad lamps
    for (uint8_t i = 0; i < INPUTS_PER_PLAYER; i++) {
        if(pinLED[i] != 255)
            gpio_put(pinLED[i], tu_bit_test(lamp.data[PLAYER_1], i + 2));
        if(pinLED[i+5] != 255)
            gpio_put(pinLED[i+5], tu_bit_test(lamp.data[PLAYER_2], i + 2));
    }
}


//--------------------------------------------------------------------
// Main
//--------------------------------------------------------------------

int main(void)
{
    board_init();
    //---------------- GPIO INPUT ----------------

    // Initialize switch pins as inputs with pull-ups
    for (int i = 0; i < INPUTS_PER_PLAYER * 2; i++)
    {
        gpio_init(pinSwitch[i]);
        gpio_set_dir(pinSwitch[i], GPIO_IN);
        gpio_pull_up(pinSwitch[i]);
    }

    //---------------- GPIO OUTPUT ----------------

    // Initialize LED pins as outputs
    for (int i = 0; i < INPUTS_PER_PLAYER * 2; i++)
    {
        gpio_init(pinLED[i]);
        gpio_set_dir(pinLED[i], GPIO_OUT);
    }

    // Initialize mux control pins as outputs and set them to 0 (default to first mux position)
    for (int i = 0; i < 2; i++)
    {
        gpio_init(pinNXP1[i]);
        gpio_set_dir(pinNXP1[i], GPIO_OUT);

        gpio_init(pinNXP2[i]);
        gpio_set_dir(pinNXP2[i], GPIO_OUT);
    }

    //---------------- USB INIT ----------------

    tud_init(BOARD_TUD_RHPORT);

    //---------------- MAIN LOOP ----------------

    while (true)
    {
        tud_task();
        piuio_task();
        hid_task();
    }
}
