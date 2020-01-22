#include "util.h"
#include <time.h>

now_t s_now_t;
now_t ms_now_t;

void util_init()
{
  s_now_t  = 1000000000U;
  ms_now_t = 1000000U;
}

now_t now()
{
  struct timespec t_cur;
  clock_gettime(CLOCK_MONOTONIC,&t_cur);
  return (now_t)t_cur.tv_sec*s_now_t+t_cur.tv_nsec;
}

