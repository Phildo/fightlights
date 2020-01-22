#ifndef SYNC_H
#define SYNC_H

#include "util.h"

//input
extern pthread_mutex_t input_lock;

extern int input_requested;
extern pthread_cond_t input_consumed_cond;

extern int io_ran_once;
extern pthread_cond_t io_ran_once_cond;

extern now_t t_tick;
extern int io_hogged_core;
extern pthread_cond_t io_forgiven_cond;

//gpu
extern pthread_mutex_t strip_lock;

extern int strip_ready;
extern pthread_cond_t strip_ready_cond;

#endif //SYNC_H

