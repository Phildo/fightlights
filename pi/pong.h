#ifndef PONG_H
#define PONG_H

#include "params.h"

extern nybl strip_leds[STRIP_NUM_LEDS];

void pong_init();
int pong_do();
void pong_kill();

#endif //PONG_H

