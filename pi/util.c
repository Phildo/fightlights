#include "util.h"
#include <time.h>
#include <stdio.h>

now_t ms_now_t;
now_t s_now_t;

void util_init()
{
  ms_now_t = 1000000U;
  s_now_t  = ms_now_t*1000U;
}

now_t now()
{
  struct timespec t_cur;
  clock_gettime(CLOCK_MONOTONIC,&t_cur);
  return (now_t)t_cur.tv_sec*s_now_t+t_cur.tv_nsec;
}

void dovprintf(const char *s)
{
  //comment out if not debugging!
  //printf(s);
  //fflush(stdout);
}

