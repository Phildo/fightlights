#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wiringserial.h"
#include "params.h"
#include "pong.h"

//serial
int fp;
byte *buff;
int buff_n;

nybl get_px(int index)
{
  nybl lut = 0;
  int i = index/2;
  if(index%2 == 0)
    lut = buff[i] & 0x0F;
  else
    lut = ((buff[i] & 0xF0) >> 4);
  return lut;
}

void set_px(int index, nybl lut)
{
  int i = index/2;
  if(index%2 == 0)
  {
    buff[i] &= 0xF0;
    buff[i] |= (byte)lut;
  }
  else
  {
    buff[i] &= 0x0F;
    buff[i] |= ((byte)lut << 4);
  }
}

//

void init_buff()
{
  buff_n = STRIP_NUM_LEDS/2+strlen(FLUSH_TRIGGER);
  buff = (byte *)malloc(sizeof(byte)*buff_n+1);
  memset(buff,0,sizeof(byte)*buff_n+1);
  strcpy(buff+(buff_n-strlen(FLUSH_TRIGGER)),FLUSH_TRIGGER);

  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
    buff[i] = 0;
    //buff[i] = (byte)rand();
    /*
    switch(i%8)
    {
      case 0: buff[i] = 0x10; break;
      case 1: buff[i] = 0x32; break;
      case 2: buff[i] = 0x54; break;
      case 3: buff[i] = 0x76; break;
      case 4: buff[i] = 0x98; break;
      case 5: buff[i] = 0xBA; break;
      case 6: buff[i] = 0xDC; break;
      case 7: buff[i] = 0xFE; break;
    }
    */
  }
}

void init_ser()
{
  fp = 0;
  fp = serialOpen(SERIAL_FILE, BAUD_RATE);
  if(!fp)
  {
    printf("could not open serial file %s",SERIAL_FILE);
    exit(1);
  }
}

void compress_strip()
{
  int buff_i  = 0;
  int strip_i = 0;
  while(strip_i < STRIP_NUM_LEDS)
  {
    buff[buff_i] = strip_leds[strip_i] | (strip_leds[strip_i+1] << 4);
    buff_i++;
    strip_i += 2;
  }
}

void iterate_strip()
{
  for(int i = 0; i < STRIP_NUM_LEDS; i++)
    set_px(i,(get_px(i)+1)%0x10);
  /*
  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
    switch(buff[i])
    {
      case 0x10: buff[i] = 0x32; break;
      case 0x32: buff[i] = 0x54; break;
      case 0x54: buff[i] = 0x76; break;
      case 0x76: buff[i] = 0x98; break;
      case 0x98: buff[i] = 0xBA; break;
      case 0xBA: buff[i] = 0xDC; break;
      case 0xDC: buff[i] = 0xFE; break;
      case 0xFE: buff[i] = 0x10; break;
    }
  }
  */
}

void push_buff()
{
  serialPutns(fp,buff,buff_n);
  serialFlush(fp);
}

int main(int argc, char **argv)
{
  init_buff();
  init_ser();
  init_pong();

  while(1)
  {
    //iterate_strip();
    int delay = loop_pong();
    compress_strip();
    push_buff();

    for(int i = 0; i < 100000*delay; i++) ;
    //usleep(1000*50);
    //sleep(1);
  }

  serialClose(fp);
}

