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
#include "gpu.h"
#include "io.h"

//threading
int main_killed;

pthread_t pong_thread;
pthread_t gpu_serial_thread;
pthread_t io_serial_thread;

//input
pthread_mutex_t input_lock;
int input_requested;
pthread_cond_t input_consumed_cond;

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

void *gpu_serial_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = gpu_do();
  }
  return 0;
}

void *io_serial_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = io_do();
  }
  return 0;
}

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

  if(pthread_mutex_init(&input_lock, NULL) != 0)
  { printf("can't init input lock\n"); exit(-1); }
  if(pthread_mutex_init(&strip_lock, NULL) != 0)
  { printf("can't init strip lock\n"); exit(-1); }

  strip_ready = 0;
  pthread_cond_init(&strip_ready_cond,NULL);

  input_requested = 0;
  pthread_cond_init(&input_consumed_cond,NULL);

  err = pthread_create(&pong_thread, NULL, &pong_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  err = pthread_create(&gpu_serial_thread, NULL, &gpu_serial_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
  err = pthread_create(&io_serial_thread, NULL, &io_serial_thread_main, NULL);
  if(err != 0) { printf("can't create thread: %s", strerror(err)); exit(-1); }
}

void kill_threads()
{
  pthread_join(io_serial_thread, NULL);
  pthread_join(gpu_serial_thread, NULL);
  pthread_join(pong_thread, NULL);

  pthread_mutex_destroy(&input_lock);
  pthread_mutex_destroy(&strip_lock);
  pthread_cond_destroy(&strip_ready_cond);
}

void multithread_main()
{
  init_sigcatch();
  util_init();
  pong_init();
  gpu_init();
  io_init();

  init_threads();

  while(!main_killed) ; //send SIGTERM to kill

  io_kill();
  gpu_kill();
  pong_kill();

  kill_threads();
}

void singlethread_main()
{
  init_sigcatch();
  util_init();
  pong_init();
  gpu_init();
  io_init();

  while(!main_killed) //send SIGTERM to kill
  {
    pong_do();
    gpu_do();
    io_do();
    for(int i = 0; i < 2000000; i++) ; //spin wait
  }

  io_kill();
  gpu_kill();
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

