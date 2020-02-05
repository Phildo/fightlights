#include "mio.h"

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

int mio_killed;

int mio_fp;
byte *mio_buff;
int mio_buff_n;

unsigned int mio_btn_a_down;
unsigned int mio_btn_b_down;

void mio_buff_init()
{
  mio_buff_n = STRIP_NUM_LEDS/2+strlen(MIO_FLUSH_TRIGGER);
  mio_buff = (byte *)malloc(sizeof(byte)*mio_buff_n+1);
  if(!mio_buff)
  {
    printf("could not alloc mio buff\n");
    exit(1);
  }
  memset(mio_buff,0,sizeof(byte)*mio_buff_n+1);
  strcpy(mio_buff+(mio_buff_n-strlen(MIO_FLUSH_TRIGGER)),MIO_FLUSH_TRIGGER);
}

void mio_ser_init()
{
  mio_fp = 0;
  mio_fp = serialOpen(MIO_SERIAL_FILE, MIO_BAUD_RATE);
  if(!mio_fp)
  {
    printf("could not open mio serial file %s",MIO_SERIAL_FILE);
    exit(1);
  }
}

void mio_push()
{
  serialPut(mio_fp,mio_buff,mio_buff_n);
  serialFlush(mio_fp);
}

void mio_pull()
{
  int c;
  c = serialGetchar(mio_fp);
  if(c != -1)
  {
    #ifdef MULTITHREAD
    pthread_mutex_lock(&input_lock);
    if(mio_killed) { pthread_mutex_unlock(&input_lock); return; }
    while(input_requested) { pthread_cond_wait(&input_consumed_cond,&input_lock); }
    if(mio_killed) { pthread_mutex_unlock(&input_lock); return; }
    #endif
         if(c & 0x0E) mio_btn_a_down = 0;
    else if(c & 0x01) mio_btn_a_down = 1;
         if(c & 0xE0) mio_btn_b_down = 0;
    else if(c & 0x10) mio_btn_b_down = 1;
    #ifdef MULTITHREAD
    pthread_mutex_unlock(&input_lock);
    #endif
  }
}

void mio_die();
//public

void mio_init()
{
  mio_btn_a_down = 0;
  mio_btn_b_down = 0;
  mio_buff_init();
  mio_ser_init();
  mio_killed = 0;
}

int mio_do()
{
  if(mio_killed) { mio_die(); return 0; }
  mio_pull();
  //mio_push();
  return 1;
}

void mio_kill()
{
  mio_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  pthread_mutex_lock(&input_lock);
  input_requested = 0;
  pthread_mutex_unlock(&input_lock);
  pthread_cond_signal(&input_consumed_cond);
  #endif
}

void mio_die()
{
  if(mio_fp) serialClose(mio_fp);
  mio_fp = 0;
  if(mio_buff) free(mio_buff);
  mio_buff = 0;
  mio_buff_n = 0;
}

