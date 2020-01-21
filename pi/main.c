#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wiringserial.h"
typedef char byte;

//strip constants
#define STRIP_NUM_LEDS 300

//sync w/ arduino
#define FLUSH_TRIGGER "FLUSH"
#define BAUD_RATE 1000000

//serial
int fp;
char filename[256];
byte *buff;
int buff_n;

int main(int argc, char **argv)
{
  fp = 0;

  buff_n = STRIP_NUM_LEDS/2+strlen(FLUSH_TRIGGER);
  buff = (byte *)malloc(sizeof(byte)*buff_n+1);
  memset(buff,0,sizeof(byte)*buff_n+1);
  strcpy(buff+(buff_n-strlen(FLUSH_TRIGGER)),FLUSH_TRIGGER);

  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
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
  }

  if(argc == 2)
  {
    strcpy(filename,argv[1]);
    fp = serialOpen(filename, BAUD_RATE);
  }
  while(!fp)
  {
    printf("file:"); fflush(stdout);

    if(fgets(filename, sizeof(filename), stdin) != filename) exit(1);
    filename[strlen(filename)-1] = '\0';
    fp = serialOpen(filename, BAUD_RATE);
  }

  int step = 0;
  while(1)
  {
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

    serialPutns(fp,buff,buff_n);
    serialFlush(fp);

    //for(int i = 0; i < 16400000; i++) ;
    usleep(1000*50);
    //sleep(1);
  }

  serialClose(fp);
}

