#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wiringSerial.h>
#include "wiringSerialEXT.h"

#include "params.h"

int io_killed;

int io_fp;
byte *io_buff;
int io_buff_n;

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

//public

void io_init()
{
  io_buff_init();
  io_ser_init();
  io_killed = 0;
}

int io_do()
{
  if(io_killed) return 0;
  usleep(1000*1); //at least 1ms
  //io_push();
  return 1;
}

void io_kill()
{
  io_killed = 1;
  if(io_fp) serialClose(io_fp);
  io_fp = 0;
  if(io_buff) free(io_buff);
  io_buff = 0;
  io_buff_n = 0;
}

