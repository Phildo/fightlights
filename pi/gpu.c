#include "gpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <wiringSerial.h>
#include "wiringSerialEXT.h"

#include "params.h"
#include "sync.h"

int gpu_killed;
extern nybl strip_leds[STRIP_NUM_LEDS];

int gpu_fp;
byte *gpu_buff;
int gpu_buff_n;

void gpu_buff_init()
{
  gpu_buff_n = STRIP_NUM_LEDS/2+strlen(GPU_FLUSH_TRIGGER);
  gpu_buff = (byte *)malloc(sizeof(byte)*gpu_buff_n+1);
  if(!gpu_buff)
  {
    printf("could not alloc gpu buff\n");
    exit(1);
  }
  memset(gpu_buff,0,sizeof(byte)*gpu_buff_n+1);
  strcpy(gpu_buff+(gpu_buff_n-strlen(GPU_FLUSH_TRIGGER)),GPU_FLUSH_TRIGGER);
}

void gpu_ser_init()
{
  gpu_fp = 0;
  gpu_fp = serialOpen(GPU_SERIAL_FILE, GPU_BAUD_RATE);
  if(!gpu_fp)
  {
    printf("could not open gpu serial file %s",GPU_SERIAL_FILE);
    exit(1);
  }
}

void gpu_push()
{
  serialPut(gpu_fp,gpu_buff,gpu_buff_n);
  serialFlush(gpu_fp);
}

void compress_strip()
{
  int gpu_buff_i  = 0;
  int strip_i = 0;
  #ifdef MULTITHREAD
  pthread_mutex_lock(&strip_lock);
  if(gpu_killed) { pthread_mutex_unlock(&strip_lock); return; }
  while(!strip_ready) pthread_cond_wait(&strip_ready_cond,&strip_lock);
  if(gpu_killed) { pthread_mutex_unlock(&strip_lock); return; }
  #endif
  while(strip_i < STRIP_NUM_LEDS)
  {
    gpu_buff[gpu_buff_i] = strip_leds[strip_i] | (strip_leds[strip_i+1] << 4);
    gpu_buff_i++;
    strip_i += 2;
  }
  strip_ready = 0;
  #ifdef MULTITHREAD
  pthread_mutex_unlock(&strip_lock);
  #endif
}

//compressed buff accessors

nybl get_px(int index)
{
  nybl lut = 0;
  int i = index/2;
  if(index%2 == 0)
    lut = gpu_buff[i] & 0x0F;
  else
    lut = ((gpu_buff[i] & 0xF0) >> 4);
  return lut;
}

void set_px(int index, nybl lut)
{
  int i = index/2;
  if(index%2 == 0)
  {
    gpu_buff[i] &= 0xF0;
    gpu_buff[i] |= (byte)lut;
  }
  else
  {
    gpu_buff[i] &= 0x0F;
    gpu_buff[i] |= ((byte)lut << 4);
  }
}

void show_lut()
{
  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
    switch(i%8)
    {
      case 0: gpu_buff[i] = 0x10; break;
      case 1: gpu_buff[i] = 0x32; break;
      case 2: gpu_buff[i] = 0x54; break;
      case 3: gpu_buff[i] = 0x76; break;
      case 4: gpu_buff[i] = 0x98; break;
      case 5: gpu_buff[i] = 0xBA; break;
      case 6: gpu_buff[i] = 0xDC; break;
      case 7: gpu_buff[i] = 0xFE; break;
    }
  }
}

void iterate_strip()
{
  for(int i = 0; i < STRIP_NUM_LEDS; i++)
    set_px(i,(get_px(i)+1)%0x10);
}

void gpu_die();
//public

void gpu_init()
{
  gpu_buff_init();
  gpu_ser_init();
  gpu_killed = 0;
}

int gpu_do()
{
  if(gpu_killed) { gpu_die(); return 0; }
  compress_strip();
  //show_lut(); //uncomment to overwrite buff showing repeating LUT
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
  #endif
}

void gpu_die()
{
  if(gpu_fp) serialClose(gpu_fp);
  gpu_fp = 0;
  if(gpu_buff) free(gpu_buff);
  gpu_buff = 0;
  gpu_buff_n = 0;
}


