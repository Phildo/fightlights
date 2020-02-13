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

#ifdef NOMIDDLEMAN
extern int btn_fd[2];
byte *btn_buff[2];
int btn_buff_n;
#else
extern int mio_fd;
byte *mio_buff;
int mio_buff_n;
#endif

volatile extern unsigned char pong_state;
#ifdef NOMIDDLEMAN
unsigned char state[2];//need separate per btn!
#else
unsigned char state;
#endif
volatile unsigned char mio_btn_down[2];

#ifdef NOMIDDLEMAN
void btn_buff_init()
{
  btn_buff_n = strlen(CMD_PREAMBLE)+3; //very little data required
  for(int i = 0; i < 2; i++)
  {
    btn_buff[i] = (byte *)malloc(sizeof(byte)*btn_buff_n+1);
    if(!btn_buff[i])
    {
      printf("could not alloc btn buff %d\n",i);
      exit(1);
    }
    memset(btn_buff[i],0,sizeof(byte)*btn_buff_n+1);
    strcpy(btn_buff[i],CMD_PREAMBLE);
    btn_buff[i][strlen(CMD_PREAMBLE)] = CMD_DATA;
  }
}
#else
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
#endif

#ifdef NOMIDDLEMAN
void btn_ser_init(int i) //just wait to be given fd by ser
{
  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  if(mio_killed) { pthread_mutex_unlock(&ser_lock); return; }
  while(!btn_fd[i]) pthread_cond_wait(&btn_ser_ready_cond[i],&ser_lock);
  if(mio_killed) { pthread_mutex_unlock(&ser_lock); return; }
  //if we got here, we're good!
  pthread_mutex_unlock(&ser_lock);
  #endif
}
#else
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
#endif

#ifdef NOMIDDLEMAN
void btn_push(int i)
{
  if(state[i] != pong_state)
  {
    state[i] = pong_state;
    unsigned char data_byte = 0;
    switch(state[i])
    {
      case STATE_SIGNUP: data_byte = 0; break;
      case STATE_PLAY:   data_byte = 1; break;
      case STATE_SCORE:  data_byte = 2; break;
    }
    btn_buff[i][btn_buff_n-2] = data_byte;
    for(int j = 0; j < 3; j++)
    {
      if(serialPut(btn_fd[i],btn_buff[i],btn_buff_n-1) == -1) ser_kill_fd(&btn_fd[i]);
    }
    serialFlush(btn_fd[i]);
  }
}
#else
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
#endif

#ifdef NOMIDDLEMAN
void btn_pull(int i)
{
  int a = serialDataAvail(btn_fd[i]);
  if(a == -1) ser_kill_fd(&btn_fd[i]);
  else if(a)
  {
    int c = serialGetchar(btn_fd[i]);
    if(c == -1) ser_kill_fd(&btn_fd[i]);
    else if(c > -1) mio_btn_down[i] = c;
  }
}
#else
void mio_pull()
{
  int a = serialDataAvail(mio_fd);
  if(a == -1) ser_kill_fd(&mio_fd);
  else if(a)
  {
    int c = serialGetchar(mio_fd);
    if(c == -1) ser_kill_fd(&mio_fd);
    else if(c > -1)
    {
           if(c & 0x0E) mio_btn_down[0] = 0;
      else if(c & 0x01) mio_btn_down[0] = 1;
           if(c & 0xE0) mio_btn_down[1] = 0;
      else if(c & 0x10) mio_btn_down[1] = 1;
    }
  }
}
#endif

void mio_die();
//public

void mio_init()
{
  for(int i = 0; i < 2; i++)
    mio_btn_down[i] = 0;
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++) state[i] = 0;
  btn_buff_init();
  #else
  state = 0;
  mio_buff_init();
  #endif
  mio_killed = 0;
}

#ifdef NOMIDDLEMAN
int btn_do(int i)
{
  if(mio_killed) { mio_die(); return 0; }
  if(!btn_fd[i]) btn_ser_init(i);
  btn_pull(i);
  btn_push(i);
  return 1;
}
#else
int mio_do()
{
  if(mio_killed) { mio_die(); return 0; }
  if(!mio_fd) mio_ser_init();
  mio_pull();
  mio_push();
  return 1;
}
#endif

void mio_kill()
{
  mio_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
    pthread_cond_signal(&btn_ser_ready_cond[i]);
  #else
  pthread_cond_signal(&mio_ser_ready_cond);
  #endif
  #endif
}

void mio_die()
{
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
  {
    if(btn_buff[i]) free(btn_buff[i]);
    btn_buff[i] = 0;
  }
  btn_buff_n = 0;
  #else
  if(mio_buff) free(mio_buff);
  mio_buff = 0;
  mio_buff_n = 0;
  #endif
}

