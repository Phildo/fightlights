#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <wiringSerial.h>
#include "wiringSerialEXT.h"

#include "params.h"
#include "util.h"
#include "sync.h"

int io_killed;

int io_fp;
byte *io_buff;
int io_buff_n;

void io_buff_init()
{
  io_buff_n = STRIP_NUM_LEDS/2+strlen(IO_FLUSH_TRIGGER);
  io_buff = (byte *)malloc(sizeof(byte)*io_buff_n+1);
  if(!io_buff)
  {
    printf("could not alloc io buff\n");
    exit(1);
  }
  memset(io_buff,0,sizeof(byte)*io_buff_n+1);
  strcpy(io_buff+(io_buff_n-strlen(IO_FLUSH_TRIGGER)),IO_FLUSH_TRIGGER);
}

void io_ser_init()
{
  io_fp = 0;
  io_fp = serialOpen(IO_SERIAL_FILE, IO_BAUD_RATE);
  if(!io_fp)
  {
    printf("could not open io serial file %s",IO_SERIAL_FILE);
    exit(1);
  }
}

void io_push()
{
  serialPut(io_fp,io_buff,io_buff_n);
  serialFlush(io_fp);
}

void io_die();
//public

void io_init()
{
  io_buff_init();
  io_ser_init();
  io_killed = 0;
}

int io_do()
{
  if(io_killed) { io_die(); return 0; }
  #ifdef MULTITHREAD
  //if input is requested, or I've been hogging the core, AND I've already run once, defer
  dovprintf("io req input\n");
  pthread_mutex_lock(&input_lock);
  dovprintf("io take input\n");
  if(io_killed) { pthread_mutex_unlock(&input_lock); return 0; }
  if(io_ran_once)
  {
    while(input_requested) { dovprintf("io defer input at request"); pthread_cond_wait(&input_consumed_cond,&input_lock); dovprintf("io continue input (at request)"); }
    if(io_killed) { pthread_mutex_unlock(&input_lock); return 0; }

    if(now()-t_tick > 8*ms_now_t) io_hogged_core = 1;
    while(io_hogged_core) { dovprintf("io defer input hogged"); pthread_cond_wait(&io_forgiven_cond,&input_lock); dovprintf("io continue input (hogged)"); }
    if(io_killed) { pthread_mutex_unlock(&input_lock); return 0; }
  }
  #endif
  usleep(1000*1); //at least 1ms
  //io_push();
  #ifdef MULTITHREAD
  if(!io_ran_once)
  {
    io_ran_once = 1;
    dovprintf("io givaway input");
    pthread_mutex_unlock(&input_lock);
    dovprintf("io signals ran");
    pthread_cond_signal(&io_ran_once_cond);
  }
  else
  {
    dovprintf("io givaway input");
    pthread_mutex_unlock(&input_lock);
  }
  #endif
  return 1;
}

void io_kill()
{
  io_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  pthread_mutex_lock(&input_lock);
  input_requested = 0;
  io_hogged_core = 0;
  pthread_mutex_unlock(&input_lock);
  pthread_cond_signal(&input_consumed_cond);
  pthread_cond_signal(&io_forgiven_cond);
  #endif
}

void io_die()
{
  if(io_fp) serialClose(io_fp);
  io_fp = 0;
  if(io_buff) free(io_buff);
  io_buff = 0;
  io_buff_n = 0;
}

