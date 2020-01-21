#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <wiringSerial.h>
#include "wiringSerialEXT.h"

/*
 * serialPut:
 *  Send an n length buffer to the serial port
 *********************************************************************************
 */

void serialPut(const int fd, const char *s, const int n)
{
  write(fd, s, n);
}

