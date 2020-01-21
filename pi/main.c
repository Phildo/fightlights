#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "params.h"
#include "pong.h"
#include "gpu.h"
#include "io.h"

pthread_t pong_thread;
pthread_t gpu_serial_thread;
pthread_t io_serial_thread;

pthread_mutex_t strip_lock;

void *pong_thread_main(void *args)
{
  int go = 1;
  while(go)
  {
    go = pong_do();
    for(int i = 0; i < 200000; i++) ;
    //usleep(1000*50);
    //sleep(1);
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
  int go = 0;
  while(go)
  {
    go = io_do();
  }
  return 0;
}

void init_threads()
{
  int err;

  if(pthread_mutex_init(&strip_lock, NULL) != 0)
  { printf("can't init strip lock\n"); exit(-1); }

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

  pthread_mutex_destroy(&strip_lock);
}

void multithread_main()
{
  pong_init();
  gpu_init();
  io_init();

  init_threads();

  while(1) ;

  io_kill();
  gpu_kill();
  pong_kill();

  kill_threads();
}

void singlethread_main()
{
  pong_init();
  gpu_init();
  io_init();

  while(1)
  {
    pong_do();
    gpu_do();
    io_do();
    for(int i = 0; i < 2000000; i++) ;
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

