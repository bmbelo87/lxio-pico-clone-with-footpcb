#ifndef TRUE_LXIO_PICO_PIUIO_CONFIG_H
#define TRUE_LXIO_PICO_PIUIO_CONFIG_H

#define INPUTS_PER_PLAYER 5
#define MULTIPLEXER_BITS 2

// Modify the arrays to edit the pinout.
// Map these according to your button pins.
static const uint8_t pinSwitch[INPUTS_PER_PLAYER * 2] = {
    0,    // P1 UL
    2,    // P1 UR
    4,    // P1 CN
    6,    // P1 DL
    8,    // P1 DR
    28,   // P2 UL
    26,   // P2 UR
    21,   // P2 CN
    19,   // P2 DL
    17    // P2 DR
    // 15,    // Service
    // 14,    // Test
    // 255,   // Coin1
    // 255    // Coin2
};

// Map these according to your LED pins.
static const uint8_t pinLED[INPUTS_PER_PLAYER * 2] = {
    1,    // P1 UL
    3,    // P1 UR
    5,    // P1 CN
    7,    // P1 DL
    9,    // P1 DR
    27,   // P2 UL
    22,   // P2 DR
    20,   // P2 CN
    18,   // P2 DL
    16    // P2 DR
};

static const uint8_t pinNXP1[MULTIPLEXER_BITS] = {
    10,    // INX0
    11     // INX1
};

static const uint8_t pinNXP2[MULTIPLEXER_BITS] = {
    14,    // INX0
    15     // INX1
};

#endif //TRUE_LXIO_PICO_PIUIO_CONFIG_H
