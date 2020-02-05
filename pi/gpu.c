#include "gpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <wiringSerial.h>
#include "wiringSerialEXT.h"

#include "params.h"
#include "sync.h"

#define STRIP_MAX_BRIGHTNESS 256
#define STRIP_MIN_BRIGHTNESS 0
typedef struct
{
  byte r;
  byte g;
  byte b;
} CRGB;
CRGB lut[16];

CRGB nCRGB(byte r, byte g, byte b)
{
  CRGB c;
  c.r = r;
  c.g = g;
  c.b = b;
  return c;
}

CRGB dampen_color(CRGB in, int amt, int maxamt)
{
  int bright = 1; //takes away the lowest fractions (turns 1/10 into 2/11)
  amt    += bright;
  maxamt += bright;
  int r = in.r*amt/maxamt;
  int g = in.g*amt/maxamt;
  int b = in.b*amt/maxamt;
  return nCRGB(r,g,b);
}

CRGB hdampen_color(CRGB in, int amt, int maxamt)
{
  int bright = 1; //takes away the lowest fractions (turns 1/10 into 2/11)
  amt    += bright;
  maxamt += bright;
  amt = maxamt-amt;
  int r = in.r;
  int g = in.g;
  int b = in.b;
  for(int i = 0; i < amt; i++)
  {
    r/=3;
    g/=3;
    b/=3;
  }
  return nCRGB(r,g,b);
}

void lut_init()
{
  //helpful constants for color-picking
  CRGB black      = nCRGB(0x00,0x00,0x00); //CRGB::Black;
  CRGB white      = nCRGB(0xFF,0xFF,0xFF); //CRGB::White;
  CRGB red        = nCRGB(0xFF,0x00,0x00); //CRGB::Red;
  CRGB green      = nCRGB(0x00,0xFF,0x00); //CRGB::Green;
  CRGB blue       = nCRGB(0x00,0x00,0xFF); //CRGB::Blue;
  CRGB yellow     = nCRGB(0xFF,0xFF,0x00); //CRGB::Yellow;
  CRGB light_blue = nCRGB(0x4E,0xFD,0xEE);
  CRGB dark_blue  = nCRGB(0x3C,0x44,0xE8);
  CRGB pink       = nCRGB(0xA6,0x11,0x27);

  lut[0x0] = black;                      //color_clear
  lut[0x1] = white;                      //color_ball{,_fade[0]}
  lut[0x2] = hdampen_color(lut[0x1],2,3); //color_ball_fade[1]
  lut[0x3] = hdampen_color(lut[0x1],1,3); //color_ball_fade[2]
  lut[0x4] = hdampen_color(lut[0x1],0,3); //color_ball_fade[3]
  lut[0x5] = blue;                       //color_a{,_fade[0]}
  lut[0x6] = hdampen_color(lut[0x5],2,3); //color_a_fade[1]
  lut[0x7] = hdampen_color(lut[0x5],1,3); //color_a_fade[2]
  lut[0x8] = hdampen_color(lut[0x5],0,3); //color_a_fade[3]
  lut[0x9] = red;                        //color_b{,_fade[0]}
  lut[0xA] = hdampen_color(lut[0x9],2,3); //color_b_fade[1]
  lut[0xB] = hdampen_color(lut[0x9],1,3); //color_b_fade[2]
  lut[0xC] = hdampen_color(lut[0x9],0,3); //color_b_fade[3]
  lut[0xD] = pink;                       //color_zone
  lut[0xE] = red;                        //color_?
  lut[0xF] = green;                      //color_?
}

int gpu_killed;
extern nybl strip_leds[STRIP_NUM_LEDS];

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
}

void gpu_ser_init() //just wait to be given fd by ser
{
  #ifdef MULTITHREAD
  pthread_mutex_lock(&ser_lock);
  if(gpu_killed) { pthread_mutex_unlock(&ser_lock); return; }
  while(!gpu_fd) pthread_cond_wait(&gpu_ser_ready_cond,&ser_lock);
  if(gpu_killed) { pthread_mutex_unlock(&ser_lock); return; }
  #endif
}

void gpu_push()
{
  serialPut(gpu_fd,gpu_buff,gpu_buff_i);
  serialFlush(gpu_fd);
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
  while(strip_i < STRIP_NUM_LEDS)
  {
    byte runlen = 1;
    int color = strip_leds[strip_i]; strip_i++;
    while(runlen < 0xFF && strip_i < STRIP_NUM_LEDS && strip_leds[strip_i] == color)
    {
      runlen++;
      strip_i++;
    }
    gpu_buff[gpu_buff_i] = runlen;       gpu_buff_i++;
    gpu_buff[gpu_buff_i] = lut[color].r; gpu_buff_i++;
    gpu_buff[gpu_buff_i] = lut[color].g; gpu_buff_i++;
    gpu_buff[gpu_buff_i] = lut[color].b; gpu_buff_i++;
    n_commands++;
    /*
    gpu_buff[gpu_buff_i] = strip_leds[strip_i] | (strip_leds[strip_i+1] << 4);
    gpu_buff_i++;
    strip_i += 2;
    */
  }
  gpu_buff[strlen(CMD_PREAMBLE)+1] = n_commands;
  gpu_buff[gpu_buff_i] = '\0';
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
  lut_init();
  gpu_buff_init();
  gpu_killed = 0;
}

int gpu_do()
{
  if(gpu_killed) { gpu_die(); return 0; }
  if(!gpu_fd) gpu_ser_init();
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
  pthread_cond_signal(&gpu_ser_ready_cond);
  #endif
}

void gpu_die()
{
  if(gpu_buff) free(gpu_buff);
  gpu_buff = 0;
  gpu_buff_n = 0;
}


