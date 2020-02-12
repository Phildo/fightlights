#ifndef SYNC_H
#define SYNC_H

#include "util.h"

//serial
extern pthread_mutex_t ser_lock;
extern pthread_cond_t ser_requested_cond;
extern pthread_cond_t gpu_ser_ready_cond;
#ifdef NOMIDDLEMAN
extern pthread_cond_t btn_ser_ready_cond[2];
#else
extern pthread_cond_t mio_ser_ready_cond;
#endif

//gpu
extern pthread_mutex_t strip_lock;
extern int strip_ready;
extern pthread_cond_t strip_ready_cond;

//state
#define STATE_SIGNUP 0
#define STATE_PLAY 1
#define STATE_SCORE 2

#endif //SYNC_H

