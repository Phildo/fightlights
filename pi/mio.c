#include "mio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "wiringSerial.h"

#include "params.h"
#include "util.h"
#include "sync.h"
#include "ser.h"

int mio_killed;

extern int mio_fd;
byte *mio_buff;
int mio_buff_n;

volatile extern unsigned char pong_state;
unsigned char state;
volatile unsigned char mio_btn_a_down;
volatile unsigned char mio_btn_b_down;

void mio_buff_init()
{
  mio_buff_n = strlen(CMD_PREAMBLE)+3; //very little data required
  mio_buff = (byte *)malloc(sizeof(byte)*mio_buff_n+1);
  if(!mio_buff)
  {
    printf("could not alloc mio buff\n");
    exit(1);
  }
  memset(mio_buff,0,sizeof(byte)*mio_buff_n+1);
  strcpy(mio_buff,CMD_PREAMBLE);
  mio_buff[strlen(CMD_PREAMBLE)] = CMD_DATA;
}

void mio_ser_init() //just wait to be given fd by ser
{
  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  if(mio_killed) { pthread_mutex_unlock(&ser_lock); return; }
  while(!mio_fd) pthread_cond_wait(&mio_ser_ready_cond,&ser_lock);
  if(mio_killed) { pthread_mutex_unlock(&ser_lock); return; }
  //if we got here, we're good!
  pthread_mutex_unlock(&ser_lock);
  #endif
}

void mio_push()
{
  if(state != pong_state)
  {
    state = pong_state;
    unsigned char data_byte = 0;
    switch(state)
    {
      case STATE_SIGNUP: data_byte = 0; break;
      case STATE_PLAY:   data_byte = 1; break;
      case STATE_SCORE:  data_byte = 2; break;
    }
    mio_buff[mio_buff_n-2] = data_byte;
    if(serialPut(mio_fd,mio_buff,mio_buff_n-1) == -1) ser_kill_fd(&mio_fd);
    serialFlush(mio_fd);
  }
}

void mio_pull()
{
  int c;
  c = serialGetchar(mio_fd);
  if(c == -1) ser_kill_fd(&mio_fd);
  else if(c > -1)
  {
         if(c & 0x0E) mio_btn_a_down = 0;
    else if(c & 0x01) mio_btn_a_down = 1;
         if(c & 0xE0) mio_btn_b_down = 0;
    else if(c & 0x10) mio_btn_b_down = 1;
  }
}

void mio_die();
//public

void mio_init()
{
  mio_btn_a_down = 0;
  mio_btn_b_down = 0;
  state = 0;
  mio_buff_init();
  mio_killed = 0;
}

int mio_do()
{
  if(mio_killed) { mio_die(); return 0; }
  if(!mio_fd) mio_ser_init();
  mio_pull();
  mio_push();
  return 1;
}

void mio_kill()
{
  mio_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  pthread_cond_signal(&mio_ser_ready_cond);
  #endif
}

void mio_die()
{
  if(mio_buff) free(mio_buff);
  mio_buff = 0;
  mio_buff_n = 0;
}

