#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "params.h"
#include "util.h"
#include "sync.h"
#include "pong.h"
#include "ser.h"
#include "gpu.h"
#include "mio.h"

//threading
int main_killed;

pthread_t pong_thread;
pthread_t ser_thread;
pthread_t gpu_thread;
#ifdef NOMIDDLEMAN
pthread_t btn_thread[2];
#else
pthread_t mio_thread;
#endif

//serial
pthread_mutex_t ser_lock;
pthread_cond_t ser_requested_cond;
pthread_cond_t gpu_ser_ready_cond;
#ifdef NOMIDDLEMAN
pthread_cond_t btn_ser_ready_cond[2];
#else
pthread_cond_t mio_ser_ready_cond;
#endif

//gpu
pthread_mutex_t strip_lock;
int strip_ready;
pthread_cond_t strip_ready_cond;

void *pong_thread_main(void *args)
{
  int go = 1;
  now_t tick = now();
  now_t test;
  now_t delta;
  now_t remaining;
  int target_ms = 16;
  //target_ms *= 10;
  while(go)
  {
    go = pong_do();
    test = now();
    delta = test-tick;
    while(delta < target_ms*ms_now_t)
    {
      remaining = (target_ms*ms_now_t-delta);
      if(remaining > 3*ms_now_t) //buffer > usleep error
      { //usleep
        int sleep_ms = (remaining/ms_now_t)-1;
        usleep(1000*sleep_ms);
      }
      test = now();
      delta = test-tick;
    }
    tick = test;
  }
  return 0;
}

void *ser_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = ser_do();
  }
  return 0;
}


void *gpu_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = gpu_do();
  }
  return 0;
}

#ifdef NOMIDDLEMAN
void *btn_thread_main(void *args)
{
  int i = (int)args;
  int go = 1;
  while(go)
  {
    go = btn_do(i);
  }
  return 0;
}
#else
void *mio_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = mio_do();
  }
  return 0;
}
#endif

void kill_main(int signum, siginfo_t *info, void *ptr) //catches ANY signal and kills self
{
  main_killed = 1;
}

void init_sigcatch()
{
  main_killed = 0;
  static struct sigaction s;
  memset(&s, 0, sizeof(s));
  s.sa_sigaction = kill_main;
  s.sa_flags = SA_SIGINFO;
  sigaction(SIGTERM, &s, NULL);
}

void init_threads()
{
  int err;

  if(pthread_mutex_init(&ser_lock, NULL) != 0)
  { printf("can't init ser lock\n"); exit(-1); }
  if(pthread_mutex_init(&strip_lock, NULL) != 0)
  { printf("can't init strip lock\n"); exit(-1); }

  pthread_cond_init(&ser_requested_cond,NULL);
  pthread_cond_init(&gpu_ser_ready_cond,NULL);
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
    pthread_cond_init(&btn_ser_ready_cond[i],NULL);
  #else
  pthread_cond_init(&mio_ser_ready_cond,NULL);
  #endif

  strip_ready = 0;
  pthread_cond_init(&strip_ready_cond,NULL);

  err = pthread_create(&pong_thread, NULL, &pong_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  err = pthread_create(&ser_thread, NULL, &ser_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  err = pthread_create(&gpu_thread, NULL, &gpu_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
  {
    err = pthread_create(&btn_thread[i], NULL, &btn_thread_main, (void *)i);
    if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  }
  #else
  err = pthread_create(&mio_thread, NULL, &mio_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  #endif
}

void kill_threads()
{
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
    pthread_join(btn_thread[i], NULL);
  #else
  pthread_join(mio_thread, NULL);
  #endif
  pthread_join(gpu_thread, NULL);
  pthread_join(ser_thread, NULL);
  pthread_join(pong_thread, NULL);

  pthread_mutex_destroy(&strip_lock);
  pthread_mutex_destroy(&ser_lock);
  pthread_cond_destroy(&strip_ready_cond);
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
    pthread_cond_destroy(&btn_ser_ready_cond[i]);
  #else
  pthread_cond_destroy(&mio_ser_ready_cond);
  #endif
  pthread_cond_destroy(&gpu_ser_ready_cond);
  pthread_cond_destroy(&ser_requested_cond);
}

void multithread_main()
{
  init_sigcatch();
  util_init();
  pong_init();
  ser_init();
  gpu_init();
  mio_init();

  init_threads();

  while(!main_killed) ; //send SIGTERM to kill

  mio_kill();
  gpu_kill();
  ser_kill();
  pong_kill();

  kill_threads();
}

void singlethread_main()
{
  init_sigcatch();
  util_init();
  pong_init();
  ser_init();
  gpu_init();
  mio_init();

  while(!main_killed) //send SIGTERM to kill
  {
    pong_do();
    ser_do();
    gpu_do();
    #ifdef NOMIDDLEMAN
    for(int i = 0; i < 2; i++)
      btn_do(i);
    #else
    mio_do();
    #endif
    for(int i = 0; i < 2000000; i++) ; //spin wait
  }

  mio_kill();
  gpu_kill();
  ser_kill();
  pong_kill();
}

int main(int argc, char **argv)
{
  #ifdef MULTITHREAD
  multithread_main();
  #else
  singlethread_main();
  #endif
}

