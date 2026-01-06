#ifndef TRUE_LXIO_PICO_PIUIO_STRUCTS_H
#define TRUE_LXIO_PICO_PIUIO_STRUCTS_H

enum state {
    PLAYER_1,
    CABINET,
    PLAYER_2,
    UNUSED
};

struct lampArray {
    union{
        uint8_t data[8];
        struct {
            // p1 cmd byte 0
            uint8_t p1_mux : 2;
            uint8_t p1_ul_light : 1;
            uint8_t p1_ur_light : 1;
            uint8_t p1_cn_light : 1;
            uint8_t p1_dl_light : 1;
            uint8_t p1_dr_light : 1;
            uint8_t empty1 : 1;

            // p1 cmd byte 1
            uint8_t empty2 : 2;
            uint8_t bass_light : 1;
            uint8_t empty3 : 3;
            uint8_t report : 1;
            uint8_t empty4 : 1;

            // p2 cmd byte 2
            uint8_t p2_mux : 2;
            uint8_t p2_ul_light : 1;
            uint8_t p2_ur_light : 1;
            uint8_t p2_cn_light : 1;
            uint8_t p2_dl_light : 1;
            uint8_t p2_dr_light : 1;
            uint8_t r2_halo : 1;

            // p2 cmd byte 3
            uint8_t r1_halo : 1;
            uint8_t l2_halo : 1;
            uint8_t l1_halo : 1;
            uint8_t counter1 : 1;
            uint8_t counter2 : 1;
            uint8_t empty5 : 1;
            uint8_t r1_halo_dupe : 1;
            uint8_t r2_halo_dupe : 1;

            uint32_t empty6 : 32;
        };
    };
};

#endif //TRUE_LXIO_PICO_PIUIO_STRUCTS_H
