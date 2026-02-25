#ifndef TRUE_LXIO_PICO_PIUIO_CONFIG_H
#define TRUE_LXIO_PICO_PIUIO_CONFIG_H

// Modify these arrays to edit the pin out.
// Map these according to your button pins.
static const uint8_t pinSwitch[10] = {
        0,     // P1 UL
        2,     // P1 UR
        4,     // P1 CN
        6,      // P1 DL
        8,      // P1 DR
        28,     // P2 UL
        26,     // P2 UR
        21,      // P2 CN
        19,      // P2 DL
        17      // P2 DR
        // 15,     // Service
        // 14,     // Test
        // 255,    // Coin1
        // 255     // Coin2
};

// Map these according to your LED pins.
static const uint8_t pinLED[10] = {
        1,     // P1 UL
        3,     // P1 UR
        5,     // P1 CN
        7,      // P1 DL
        9,      // P1 DR
        27,     // P2 UL
        22,     // P2 DR
        20,      // P2 CN
        18,      // P2 DL
        16       // P2 DR
};

static const uint8_t pinNXP1[2] = {
        10,     // INX0
        11      // INX1
};

static const uint8_t pinNXP2[2] = {
        14,     // INX0
        15      // INX1
};

// Map this to the switch to change between PIUIO and LXIO
static const uint8_t pinled = 25;

#endif //TRUE_LXIO_PICO_PIUIO_CONFIG_H
