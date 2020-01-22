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

unsigned int io_btn_a_down;
unsigned int io_btn_b_down;

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

void io_pull()
{
  int c;
  c = serialGetchar(io_fp);
  if(c != -1)
  {
    #ifdef MULTITHREAD
    pthread_mutex_lock(&input_lock);
    if(io_killed) { pthread_mutex_unlock(&input_lock); return; }
    while(input_requested) { pthread_cond_wait(&input_consumed_cond,&input_lock); }
    if(io_killed) { pthread_mutex_unlock(&input_lock); return; }
    #endif
         if(c & 0x0E) io_btn_a_down = 0;
    else if(c & 0x01) io_btn_a_down = 1;
         if(c & 0xE0) io_btn_b_down = 0;
    else if(c & 0x10) io_btn_b_down = 1;
    #ifdef MULTITHREAD
    pthread_mutex_unlock(&input_lock);
    #endif
  }
}

void io_die();
//public

void io_init()
{
  io_btn_a_down = 0;
  io_btn_b_down = 0;
  io_buff_init();
  io_ser_init();
  io_killed = 0;
}

int io_do()
{
  if(io_killed) { io_die(); return 0; }
  io_pull();
  //io_push();
  return 1;
}

void io_kill()
{
  io_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  pthread_mutex_lock(&input_lock);
  input_requested = 0;
  pthread_mutex_unlock(&input_lock);
  pthread_cond_signal(&input_consumed_cond);
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

