#ifndef TRUE_LXIO_PICO_PIUIO_CONFIG_H
#define TRUE_LXIO_PICO_PIUIO_CONFIG_H

// Modify these arrays to edit the pin out.
// Map these according to your button pins.
static const uint8_t pinSwitch[10] = {
        0,     // P1 DL
        2,     // P1 UL
        4,     // P1 CN
        6,      // P1 UR
        8,      // P1 DR
        28,     // P2 DL
        26,     // P2 UL
        21,      // P2 CN
        19,      // P2 UR
        17      // P2 DR
        // 15,     // Service
        // 14,     // Test
        // 255,    // Coin1
        // 255     // Coin2
};

// Map these according to your LED pins.
static const uint8_t pinLED[10] = {
        1,     // P1 DL
        3,     // P1 UL
        5,     // P1 CN
        7,      // P1 UR
        9,      // P1 DR
        27,     // P2 DL
        22,     // P2 UL
        20,      // P2 CN
        18,      // P2 UR
        16       // P2 DR
};

// Map this to the switch to change between PIUIO and LXIO
static const uint8_t pinled = 25;

#endif //TRUE_LXIO_PICO_PIUIO_CONFIG_H
