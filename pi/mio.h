#ifndef MIO_H
#define MIO_H

#include "params.h"

void mio_init();
#ifdef NOMIDDLEMAN
int btn_do(int i);
#else
int mio_do();
#endif
void mio_kill();

#endif //MIO_H

