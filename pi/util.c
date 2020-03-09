#include "util.h"
#include <time.h>
#include <stdio.h>
#include "params.h"

//color
color ncolor(byte r, byte g, byte b)
{
  color c;
  c.r = r;
  c.g = g;
  c.b = b;
  return c;
}

extern unsigned int strip_brightness;
color brightness_color(color in) //for global "strip brightness"
{
  int r = in.r*strip_brightness/STRIP_MAX_BRIGHTNESS;
  int g = in.g*strip_brightness/STRIP_MAX_BRIGHTNESS;
  int b = in.b*strip_brightness/STRIP_MAX_BRIGHTNESS;
  return ncolor((byte)r,(byte)g,(byte)b);
}

color dampen_color(color in, float amt)
{
  byte r = in.r*amt;
  byte g = in.g*amt;
  byte b = in.b*amt;
  return ncolor(r,g,b);
}

color hdampen_color(color in, int amt)
{
  float r = in.r;
  float g = in.g;
  float b = in.b;
  for(int i = 0; i < amt; i++)
  {
    r/=2;
    g/=2;
    b/=2;
  }
  return ncolor(r,g,b);
}

color mix_color(color a, color b)
{
  color c;
  int sumr = a.r+b.r;
  c.r = a.r*(float)a.r/sumr + b.r*(float)b.r/sumr;
  int sumg = a.g+b.g;
  c.g = a.g*(float)a.g/sumg + b.g*(float)b.g/sumg;
  int sumb = a.b+b.b;
  c.b = a.b*(float)a.b/sumb + b.b*(float)b.b/sumb;
  return c;
}

int color_cmp(color a, color b)
{
  return a.r == b.r &&
         a.g == b.g &&
         a.b == b.b;
}

//math
int clamp(int v, int min, int max)
{
  if(v < min) return min;
  if(v > max) return max;
  return v;
}

//timing
now_t ms_now_t;
now_t s_now_t;

void util_init()
{
  ms_now_t = 1000000U;
  s_now_t  = ms_now_t*1000U;
}

now_t now()
{
  struct timespec t_cur;
  clock_gettime(CLOCK_MONOTONIC,&t_cur);
  return (now_t)t_cur.tv_sec*s_now_t+t_cur.tv_nsec;
}

void dovprintf(const char *s)
{
  //comment out if not debugging!
  //printf(s);
  //fflush(stdout);
}

