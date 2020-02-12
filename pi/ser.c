#include "ser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "wiringSerial.h"

#include "params.h"
#include "sync.h"

int gpu_fd;
#ifdef NOMIDDLEMAN
int btn_fd[2];
#else
int mio_fd;
#endif

int ser_killed;

char *file[]    = {"/dev/ttyUSB0","/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4"};
int file_used[] = {             0,             0,             0,             0,             0};

int cmd_len;
char *whoru;
int rsp_len;
char *rsp;

void ser_die();
//public

void ser_kill_fd(int *fd)
{
  serialClose(*fd);
  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  #endif
  for(int i = 0; i < sizeof(file)/sizeof(char *); i++)
    if(file_used[i] == *fd) file_used[i] = 0;
  *fd = 0;
  #ifdef MULTITHREAD
  pthread_mutex_unlock(&ser_lock);
  pthread_cond_signal(&ser_requested_cond);
  #endif
}

void ser_init()
{
  gpu_fd = 0;
  #ifdef NOMIDDLEMAN
  btn_fd[0] = 0;
  btn_fd[1] = 0;
  #else
  mio_fd = 0;
  #endif

  //silly formatting of cmd
  cmd_len = strlen(CMD_PREAMBLE)+1; //preamble + cmd
  whoru = malloc(cmd_len+1);
  strcpy(whoru,CMD_PREAMBLE);
  whoru[cmd_len-1] = CMD_WHORU;
  whoru[cmd_len  ] = '\0';

  rsp_len = 3;
  rsp = malloc(rsp_len+1); //should be > enough for whoru
  rsp[rsp_len] = '\0';

  ser_killed = 0;
}

int ser_do()
{
  if(ser_killed) { ser_die(); return 0; }

  int fd;

  //run through available files

  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  if(ser_killed) { pthread_mutex_unlock(&ser_lock); return 0; }
  #ifdef NOMIDDLEMAN
  while(gpu_fd && btn_fd[0] && btn_fd[1] && !ser_killed) { pthread_cond_wait(&ser_requested_cond,&ser_lock); } //uniquely needs "ser_killed", because shouldn't lie about fds, even in death
  #else
  while(gpu_fd && mio_fd && !ser_killed) { pthread_cond_wait(&ser_requested_cond,&ser_lock); } //uniquely needs "ser_killed", because shouldn't lie about fds, even in death
  #endif
  if(ser_killed) { pthread_mutex_unlock(&ser_lock); return 0; }
  // do nothing and immediately unlock (lock is purely for sake of sleeping til command)
  pthread_mutex_unlock(&ser_lock);
  #endif

  for(int i = 0; i < sizeof(file)/sizeof(char *); i++)
  {
    if(file_used[i]) continue;
    fd = serialOpen(file[i], BAUD_RATE);
    if(fd) sleep(3);
    while(fd)
    {
      if(serialPut(fd,whoru,cmd_len) == -1) { serialClose(fd); fd = 0; break; }
      serialFlush(fd);
      usleep(1000*10); //10ms

      int c;
      int rsp_i = 0;
      c = serialGetchar(fd);
      while(c > -1)
      {
        rsp[rsp_i] = (char)c; rsp_i++;
        if(rsp_i == rsp_len) break;
        c = serialGetchar(fd);
      }
      if(c == -1) { serialClose(fd); fd = 0; break; }
      serialFlush(fd);

      if(!gpu_fd && strcmp(rsp,GPU_AID) == 0)
      {
        #ifdef MULTITHREAD
        pthread_mutex_lock(&ser_lock);
        #endif
        gpu_fd = fd;
        file_used[i] = fd;
        fd = 0;
        #ifdef MULTITHREAD
        pthread_mutex_unlock(&ser_lock);
        pthread_cond_signal(&gpu_ser_ready_cond);
        #endif
      }
      #ifdef NOMIDDLEMAN
      else if(!btn_fd[0] && strcmp(rsp,BTN0_AID) == 0)
      {
        #ifdef MULTITHREAD
        pthread_mutex_lock(&ser_lock);
        #endif
        btn_fd[0] = fd;
        file_used[i] = fd;
        fd = 0;
        #ifdef MULTITHREAD
        pthread_mutex_unlock(&ser_lock);
        pthread_cond_signal(&btn_ser_ready_cond[0]);
        #endif
      }
      else if(!btn_fd[1] && strcmp(rsp,BTN1_AID) == 0)
      {
        #ifdef MULTITHREAD
        pthread_mutex_lock(&ser_lock);
        #endif
        btn_fd[1] = fd;
        file_used[i] = fd;
        fd = 0;
        #ifdef MULTITHREAD
        pthread_mutex_unlock(&ser_lock);
        pthread_cond_signal(&btn_ser_ready_cond[1]);
        #endif
      }
      #else
      else if(!mio_fd && strcmp(rsp,MIO_AID) == 0)
      {
        #ifdef MULTITHREAD
        pthread_mutex_lock(&ser_lock);
        #endif
        mio_fd = fd;
        file_used[i] = fd;
        fd = 0;
        #ifdef MULTITHREAD
        pthread_mutex_unlock(&ser_lock);
        pthread_cond_signal(&mio_ser_ready_cond);
        #endif
      }
      #endif
    }
  }
  //if failed, will be triggered again
  sleep(3);

  return 1;
}

void ser_kill()
{
  ser_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  /*
  //actually, DON'T lie about this (will cause erroneous serial cleanup)
  //leaving (commented out) code here so its absence isn't interpreted as an omission
  pthread_mutex_lock(&ser_lock);
  gpu_fd = 1;
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
    btn_fd[i] = 1;
  #else
  mio_fd = 1;
  #endif
  pthread_mutex_unlock(&ser_lock);
  */
  pthread_cond_signal(&ser_requested_cond);
  #endif
}

void ser_die()
{
  if(gpu_fd) serialClose(gpu_fd);
  gpu_fd = 0;
  #ifdef NOMIDDLEMAN
  for(int i = 0; i < 2; i++)
  {
    if(btn_fd[i]) serialClose(btn_fd[i]);
    btn_fd[i] = 0;
  }
  #else
  if(mio_fd) serialClose(mio_fd);
  mio_fd = 0;
  #endif
}

