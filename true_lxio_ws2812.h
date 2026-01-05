#ifndef TRUE_LXIO_PICO_PIUIO_WS2812_H
#define TRUE_LXIO_PICO_PIUIO_WS2812_H

#include "true_lxio_structs.h"

void ws2812_init(struct lampArray* l);
void ws2812_lock_mtx();
void ws2812_unlock_mtx();

#endif //TRUE_LXIO_PICO_PIUIO_WS2812_H
