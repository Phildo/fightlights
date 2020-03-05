#include "gpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "wiringSerial.h"

#include "params.h"
#include "util.h"
#include "sync.h"
#include "ser.h"

int gpu_killed;
volatile extern color strip_leds[STRIP_NUM_LEDS];
unsigned int strip_brightness; //1-10

extern int gpu_fd;
byte *gpu_buff;
int gpu_buff_n;
int gpu_buff_i;

void gpu_buff_init()
{
  gpu_buff_n = strlen(CMD_PREAMBLE)+2+STRIP_NUM_LEDS*4; //assuming single-led RLE (would be VERY inefficient)
  gpu_buff = (byte *)malloc(sizeof(byte)*gpu_buff_n+1);
  if(!gpu_buff)
  {
    printf("could not alloc gpu buff\n");
    exit(1);
  }
  memset(gpu_buff,0,sizeof(byte)*gpu_buff_n+1);
  strcpy(gpu_buff,CMD_PREAMBLE);
  gpu_buff[strlen(CMD_PREAMBLE)] = CMD_DATA;

  strip_brightness = 1; //min- don't blind people by default
}

void gpu_ser_init() //just wait to be given fd by ser
{
  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  if(gpu_killed) { pthread_mutex_unlock(&ser_lock); return; }
  while(!gpu_fd) pthread_cond_wait(&gpu_ser_ready_cond,&ser_lock);
  if(gpu_killed) { pthread_mutex_unlock(&ser_lock); return; }
  //if we got here, we're good!
  pthread_mutex_unlock(&ser_lock);
  #endif
}

void gpu_push()
{
  if(serialPut(gpu_fd,gpu_buff,gpu_buff_i) == -1) ser_kill_fd(&gpu_fd);
  else serialFlush(gpu_fd);
}

void compress_strip()
{
  gpu_buff_i = strlen(CMD_PREAMBLE)+1+1; //preamble assumed already in place, also jump 'ncommands' byte (set at end)
  int strip_i = 0;
  byte n_commands = 0;
  #ifdef MULTITHREAD
  pthread_mutex_lock(&strip_lock);
  if(gpu_killed) { pthread_mutex_unlock(&strip_lock); return; }
  while(!strip_ready) pthread_cond_wait(&strip_ready_cond,&strip_lock);
  if(gpu_killed) { pthread_mutex_unlock(&strip_lock); return; }
  #endif

  int cmd_enc_i;
  int cmd_n_i;
  int stream_len;
  int run_len;
  while(strip_i < STRIP_NUM_LEDS)
  {
    //begin cmd, assume stream
    n_commands++;
    gpu_buff[gpu_buff_i] = ENC_STREAM; cmd_enc_i = gpu_buff_i; gpu_buff_i++;
    gpu_buff[gpu_buff_i] = 1;          cmd_n_i   = gpu_buff_i; gpu_buff_i++;
    color c = brightness_color(strip_leds[strip_i]); strip_i++;
    gpu_buff[gpu_buff_i] = c.r; gpu_buff_i++;
    gpu_buff[gpu_buff_i] = c.g; gpu_buff_i++;
    gpu_buff[gpu_buff_i] = c.b; gpu_buff_i++;
    run_len = 1;
    stream_len = 1;

    while(stream_len < 0xFF && strip_i < STRIP_NUM_LEDS)
    {
      color nc = brightness_color(strip_leds[strip_i]); strip_i++;
      stream_len++;
      if(color_cmp(nc,c)) run_len++;
      else                run_len = 1;
      if(run_len == 3)
      {
        stream_len -= 3;
        if(stream_len)
        {
          gpu_buff[cmd_n_i] = stream_len;
          stream_len = 0;
          gpu_buff_i -= 3*2;
          n_commands++;
          gpu_buff[gpu_buff_i] = ENC_RUN; cmd_enc_i = gpu_buff_i; gpu_buff_i++;
          gpu_buff[gpu_buff_i] = run_len; cmd_n_i   = gpu_buff_i; gpu_buff_i++; //will get overwritten later anyways
          gpu_buff[gpu_buff_i] = nc.r; gpu_buff_i++;
          gpu_buff[gpu_buff_i] = nc.g; gpu_buff_i++;
          gpu_buff[gpu_buff_i] = nc.b; gpu_buff_i++;
        }
        else
        {
          gpu_buff[cmd_enc_i] = ENC_RUN;
          gpu_buff_i -= 3;
        }
        while(run_len < 0xFF && strip_i < STRIP_NUM_LEDS && color_cmp(brightness_color(strip_leds[strip_i]),c))
        {
          run_len++;
          strip_i++;
        }
        gpu_buff[cmd_n_i] = run_len;
        run_len = 0;
        break;
      }
      else
      {
        gpu_buff[gpu_buff_i] = nc.r; gpu_buff_i++;
        gpu_buff[gpu_buff_i] = nc.g; gpu_buff_i++;
        gpu_buff[gpu_buff_i] = nc.b; gpu_buff_i++;
      }
      c = nc;
    }
    if(stream_len) gpu_buff[cmd_n_i] = stream_len;
  }
  gpu_buff[strlen(CMD_PREAMBLE)+1] = n_commands;
  gpu_buff[gpu_buff_i] = '\0';
  strip_ready = 0;
  #ifdef MULTITHREAD
  pthread_mutex_unlock(&strip_lock);
  #endif
  //printf("%d\n",gpu_buff_i);
  /*
  printf("BEGIN\n");
  for(int i = 0; i < gpu_buff_i; i++)
    printf("%03d %02x %c\n", gpu_buff[i], gpu_buff[i], gpu_buff[i]);
  printf("END\n");
  printf("\n");
  */
}

void gpu_die();
//public

void gpu_init()
{
  gpu_buff_init();
  gpu_killed = 0;
}

int gpu_do()
{
  if(gpu_killed) { gpu_die(); return 0; }
  if(!gpu_fd) gpu_ser_init();
  compress_strip();
  gpu_push();
  return 1;
}

void gpu_kill()
{
  gpu_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  pthread_mutex_lock(&strip_lock);
  strip_ready = 1;
  pthread_mutex_unlock(&strip_lock);
  pthread_cond_signal(&strip_ready_cond);
  pthread_cond_signal(&gpu_ser_ready_cond);
  #endif
}

void gpu_die()
{
  if(gpu_buff) free(gpu_buff);
  gpu_buff = 0;
  gpu_buff_n = 0;
}

