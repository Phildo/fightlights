#include "pong.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include "util.h"
#include "sync.h"

//logic
#define MAX_HIT_ZONE 30 //measured in real LEDs
#define MIN_HIT_ZONE 4  //measured in real LEDs
#define STRIP_NUM_VIRTUAL_PER_LED 10
#define STRIP_NUM_VIRTUAL_LEDS (STRIP_NUM_LEDS*STRIP_NUM_VIRTUAL_PER_LED)

#define SCORE_T 100

int pong_killed;

static unsigned int t_mod_twelvepi_100;
volatile unsigned char pong_state;
static unsigned int state_t;
static int speed;
static int zone_a_len;
static int zone_b_len;
static long prev_virtual_ball_p;
static long virtual_ball_p;
static long prev_ball_p;
static long ball_p;
volatile int pong_serve;
static int server;
static int bounce;
volatile extern unsigned char mio_btn_down[2];
static unsigned char btn_a_down;
static unsigned int btn_a_down_t;
static unsigned int btn_a_press_t;
static unsigned int btn_a_up_t;
static unsigned char btn_b_down;
static unsigned int btn_b_down_t;
static unsigned int btn_b_press_t;
static unsigned int btn_b_up_t;
static int btn_a_hit_p;
static int missile_a_hit_p;
static unsigned int missile_a_hit_t;
static int btn_b_hit_p;
static int missile_b_hit_p;
static unsigned int missile_b_hit_t;
volatile extern int snd_play;

//vis state
//signup

static color clear;
static color black;
static color white;
static color red;
static color green;
static color blue;
static color yellow;
static color light_blue;
static color dark_blue;
static color pink;

static color color_a;
static color color_b;
static color color_ball;
static color color_zone;

volatile color strip_leds[STRIP_NUM_LEDS];

void pong_colors_init()
{
  color clear      = ncolor(0x00,0x00,0x00);
  color black      = clear;
  color white      = ncolor(0xFF,0xFF,0xFF);
  color red        = ncolor(0xFF,0x00,0x00);
  color green      = ncolor(0x00,0xFF,0x00);
  color blue       = ncolor(0x00,0x00,0xFF);
  color yellow     = ncolor(0xFF,0xFF,0x00);
  color light_blue = ncolor(0x4E,0xFD,0xEE);
  color dark_blue  = ncolor(0x3C,0x44,0xE8);
  color pink       = ncolor(0xA6,0x11,0x27);

  color_a = red;
  color_b = blue;
  color_ball = white;
  color_zone = pink;
}

int back(int i)
{
  return STRIP_NUM_LEDS-1-i;
}

int vback(int i)
{
  return STRIP_NUM_VIRTUAL_LEDS-1-i;
}

float timed_pwave(int i, float speed, float invwavelength)
{
  return 0.5+sin(12.f*314.f-t_mod_twelvepi_100*speed+i*invwavelength)*0.5;
}

void draw_base_strip()
{
  int i = 0;
  for(; i < zone_a_len; i++)
  {
    float d = timed_pwave(i,0.1f,0.5f);
    strip_leds[i] = dampen_color(color_zone,0.5+(d*d*d)/2.0);
  }
  for(; i < STRIP_NUM_LEDS-zone_b_len; i++)
    strip_leds[i] = clear;
  for(; i < STRIP_NUM_LEDS; i++)
  {
    float d = timed_pwave(STRIP_NUM_LEDS-i-1,0.1f,0.5f);
    strip_leds[i] = dampen_color(color_zone,0.5+(d*d*d)/2.0);
  }
}

void fill_virtual_range(color c, int vfrom, int vto)
{
  const float v = STRIP_NUM_VIRTUAL_PER_LED; //shorthand conversion
  int from = vfrom/STRIP_NUM_VIRTUAL_PER_LED;
  int to   = vto  /STRIP_NUM_VIRTUAL_PER_LED;
  int fmv = vfrom%STRIP_NUM_VIRTUAL_PER_LED;
  int tmv =   vto%STRIP_NUM_VIRTUAL_PER_LED;
  if(vfrom > vto)
  { //swap em
    int t = from;
    from = to;
    to = t;
    t = fmv;
    fmv = tmv;
    tmv = t;
  }
  int i = from;
  if(to == from) strip_leds[i] = mix_color(strip_leds[i],dampen_color(c,(tmv-fmv+1)/v));
  else
  {
                       { strip_leds[i] = mix_color(strip_leds[i],dampen_color(c,(v-fmv)/v)); i++; } //first spot
    for(; i < to; i++) { strip_leds[i] = mix_color(strip_leds[i],c); } //certainly full
                       { strip_leds[i] = mix_color(strip_leds[i],dampen_color(c,(tmv+1)/v)); } //final spot
  }
}

void draw_ball()
{
  //fill out range
  //fill_virtual_range(color_ball, prev_virtual_ball_p, virtual_ball_p);
  //fill out AA ball
  fill_virtual_range(color_ball, fmax(0,virtual_ball_p-STRIP_NUM_VIRTUAL_PER_LED/2),fmin(virtual_ball_p+STRIP_NUM_VIRTUAL_PER_LED/2,STRIP_NUM_VIRTUAL_LEDS-1));
}

void set_state(unsigned char s);
void pong_init()
{
  //strip
  pong_colors_init();

  btn_a_down = 0;
  btn_a_down_t = 0;
  btn_a_press_t = 0;
  btn_a_up_t = 0;
  missile_a_hit_t = 0;
  btn_b_down = 0;
  btn_b_down_t = 0;
  btn_b_press_t = 0;
  btn_b_up_t = 0;
  missile_b_hit_t = 0;

  //vis state
  //signup

  t_mod_twelvepi_100 = 0;

  set_state(STATE_SIGNUP);
  //set_state(STATE_DEBUG); //HACK
  draw_base_strip();

  pong_killed = 0;
}

void set_state(unsigned char s)
{
  pong_state = s;
  state_t = 0;
  switch(pong_state)
  {
    case STATE_SIGNUP:
    {
      snd_play = 1;
      if(btn_a_down_t >= btn_b_down_t)
      {
        btn_a_down_t  = 1;
        btn_b_down_t  = 0;
      }
      else
      {
        btn_a_down_t  = 0;
        btn_b_down_t  = 1;
      }
      speed = 10;
      zone_a_len = MAX_HIT_ZONE;
      zone_b_len = MAX_HIT_ZONE;
      btn_a_hit_p     = -1;
      missile_a_hit_p = -1;
      btn_b_hit_p     = -1;
      missile_b_hit_p = -1;
      bounce = 0;
    }
      break;
    case STATE_PLAY:
    {
      snd_play = 2;
      if(btn_a_down_t >= btn_b_down_t)
      {
        server = 1;
        virtual_ball_p = 0;
        prev_virtual_ball_p = virtual_ball_p;
        ball_p = 0;
        prev_ball_p = ball_p;
      }
      else
      {
        server = -1;
        virtual_ball_p = STRIP_NUM_VIRTUAL_LEDS-1;
        prev_virtual_ball_p = virtual_ball_p;
        ball_p = back(0);
        prev_ball_p = ball_p;
      }
      pong_serve = server;
    }
      break;
    case STATE_SCORE:
    {
      snd_play = 2;
    }
      break;
    case STATE_DEBUG:
    {
      speed = 11;
      server = 1;
      virtual_ball_p = 0;
      prev_virtual_ball_p = virtual_ball_p;
      ball_p = 0;
      prev_ball_p = ball_p;
      pong_serve = server;
    }
      break;
  }
}

void pong_die();
//public

int pong_do()
{
  if(pong_killed) { pong_die(); return 0; }

  t_mod_twelvepi_100 = (t_mod_twelvepi_100+1)%(12*314);

  //get mio
  btn_a_down = mio_btn_down[0];
  btn_b_down = mio_btn_down[1];

  //read buttons
  if(btn_a_down) { btn_a_down_t++;   btn_a_up_t = 0; }
  else           { btn_a_down_t = 0; btn_a_up_t++;   }
  if(btn_a_press_t) btn_a_press_t++;
  if(btn_a_down_t == 1 && btn_a_hit_p == -1) btn_a_press_t = 1;
  if(missile_a_hit_t) missile_a_hit_t++;

  if(btn_b_down) { btn_b_down_t++;   btn_b_up_t = 0; }
  else           { btn_b_down_t = 0; btn_b_up_t++;   }
  if(btn_b_press_t) btn_b_press_t++;
  if(btn_b_down_t == 1 && btn_b_hit_p == -1) btn_b_press_t = 1;
  if(missile_b_hit_t) missile_b_hit_t++;

  state_t++; if(state_t == 0) state_t = -1; //keep at max

  //update
  switch(pong_state)
  {
    case STATE_SIGNUP:
    {
      int t = btn_a_down_t;
      if(btn_b_down_t < btn_a_down_t) t = btn_b_down_t;
      if(t >= STRIP_NUM_LEDS/4) set_state(STATE_PLAY);
    }
      break;
    case STATE_PLAY:
    {
      //update ball
      prev_virtual_ball_p = virtual_ball_p;
      virtual_ball_p += pong_serve*speed;
      prev_ball_p = ball_p;
      ball_p = virtual_ball_p*STRIP_NUM_LEDS/STRIP_NUM_VIRTUAL_LEDS;
      if(ball_p >= STRIP_NUM_LEDS) { ball_p = back(0); set_state(STATE_SCORE); break; }
      else if(ball_p < 0)          { ball_p = 0;       set_state(STATE_SCORE); break; }

      //clear hits at midpoint
      int midpoint = STRIP_NUM_LEDS/2;
           if(pong_serve ==  1 && prev_ball_p < midpoint && ball_p >= midpoint) { btn_b_hit_p = -1; missile_b_hit_p = -1; }
      else if(pong_serve == -1 && prev_ball_p > midpoint && ball_p <= midpoint) { btn_a_hit_p = -1; missile_a_hit_p = -1; }

      //handle hits
      int should_bounce = 0;
      if(btn_a_hit_p == -1 && btn_a_down_t == 1) btn_a_hit_p = ball_p;
      if(btn_b_hit_p == -1 && btn_b_down_t == 1) btn_b_hit_p = ball_p;

      if(pong_serve == -1 && btn_a_press_t < zone_a_len && ball_p <= btn_a_press_t)
      {
        missile_a_hit_p = ball_p;
        missile_a_hit_t = 1;
        should_bounce = 1;
      }
      if(pong_serve == 1 && btn_b_press_t < zone_b_len && ball_p >= back(btn_b_press_t))
      {
        missile_b_hit_p = ball_p;
        missile_b_hit_t = 1;
        should_bounce = 1;
      }

      if(should_bounce)
      {
        snd_play = 0;
        bounce++;
        speed = 10+(bounce*10/6);
             if(pong_serve == -1) { pong_serve =  1; zone_a_len = MAX_HIT_ZONE-((bounce+2)/3); } //a served
        else if(pong_serve ==  1) { pong_serve = -1; zone_b_len = MAX_HIT_ZONE-((bounce+2)/3); } //b served
        if(zone_a_len < MIN_HIT_ZONE) zone_a_len = MIN_HIT_ZONE;
        if(zone_b_len < MIN_HIT_ZONE) zone_b_len = MIN_HIT_ZONE;
      }
    }
      break;
    case STATE_SCORE:
    {
      if(state_t > SCORE_T) set_state(STATE_SIGNUP);
    }
      break;
    case STATE_DEBUG:
    {
      prev_virtual_ball_p = virtual_ball_p;
      virtual_ball_p += pong_serve*speed;
      prev_ball_p = ball_p;
      ball_p = virtual_ball_p/STRIP_NUM_VIRTUAL_PER_LED;

      int should_bounce = 0;
      if(virtual_ball_p >= STRIP_NUM_VIRTUAL_LEDS) { virtual_ball_p = vback(0); should_bounce = 1; }
      else if(virtual_ball_p < 0)                  { virtual_ball_p = 0;        should_bounce = 1; }

      if(should_bounce)
      {
        bounce++;
             if(pong_serve == -1) { pong_serve =  1;  } //a served
        else if(pong_serve ==  1) { pong_serve = -1;  } //b served
      }
    }
      break;
  }

  #ifdef MULTITHREAD
  pthread_mutex_lock(&strip_lock);
  if(pong_killed) { pthread_mutex_unlock(&strip_lock); return 0; }
  #endif
  //draw
  //for(int i = 0; i < 10; i++) //used to test performance
  {
    switch(pong_state)
    {
      case STATE_SIGNUP:
      {
        draw_base_strip();

        int resolution = 10;
        float wspeed = 0.5f;
        float invwavelength = 0.1f;
        for(int i = 0; i < btn_a_down_t*2 && i < STRIP_NUM_LEDS/2; i++)
        {
          float d = timed_pwave(i,wspeed,invwavelength);
          d = 0.5+d*0.5;
          d = floor(d*resolution)/resolution;
          strip_leds[i] = mix_color(strip_leds[i],dampen_color(color_a,d));
        }
        for(int i = 0; i < btn_b_down_t*2 && i < STRIP_NUM_LEDS/2; i++)
        {
          float d = timed_pwave(i,wspeed,invwavelength);
          d = 0.5+d*0.5;
          d = floor(d*resolution)/resolution;
          strip_leds[back(i)] = mix_color(strip_leds[back(i)],dampen_color(color_b,d));
        }

        //static down indicator
        if(btn_a_down_t)
        {
          strip_leds[0] = color_a;
          strip_leds[back(MAX_HIT_ZONE)] = color_a;
        }
        if(btn_b_down_t)
        {
          strip_leds[MAX_HIT_ZONE] = color_b;
          strip_leds[back(0)] = color_b;
        }
      }
        break;
      case STATE_PLAY:
      {
        draw_base_strip();
        //for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear; //HACK

        int t = missile_a_hit_t;
        if(!missile_a_hit_t || (missile_a_hit_t && missile_b_hit_t && missile_a_hit_t > missile_b_hit_t)) t = missile_b_hit_t; //t == the lowest non-zero missile_[ab]_hit_t, or 0
        //if(t && t < VIRTUAL_STRIP_FADE_N) clear = color_ball_fade[(VIRTUAL_STRIP_FADE_N-t)/VIRTUAL_STRIP_FADE_N_MUL];

        //lose size flash
        if(bounce%3)
        {
          int flash = 10;
          if(pong_serve == 1 && missile_a_hit_t < flash*2)
          {
            if(missile_a_hit_t > flash && (missile_a_hit_t/2)%2) strip_leds[zone_a_len] = red;
            else                                                 strip_leds[zone_a_len] = color_zone;
          }
          if(pong_serve == -1 && missile_b_hit_t < flash*2)
          {
            if(missile_b_hit_t > flash && (missile_b_hit_t/2)%2) strip_leds[back(zone_b_len)] = red;
            else                                                 strip_leds[back(zone_b_len)] = color_zone;
          }
        }

        //draw hits
        if(missile_a_hit_p != -1) strip_leds[missile_a_hit_p] = color_a;
        if(missile_b_hit_p != -1) strip_leds[missile_b_hit_p] = color_b;

        //particles
        int off;
        int flash = 10;
        off = missile_a_hit_t/2;
        if(missile_a_hit_t && off < flash*3)
        {
          off = off%flash;
          if(missile_a_hit_p-off >= 0)              strip_leds[missile_a_hit_p-off] = dampen_color(color_ball,(flash-1.0-off)/flash);
          if(missile_a_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_a_hit_p+off] = dampen_color(color_ball,(flash-1.0-off)/flash);
        }
        off = missile_b_hit_t/2;
        if(missile_b_hit_t && off < flash*3)
        {
          off = off%flash;
          if(missile_b_hit_p-off >= 0)              strip_leds[missile_b_hit_p-off] = dampen_color(color_ball,(flash-1.0-off)/flash);
          if(missile_b_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_b_hit_p+off] = dampen_color(color_ball,(flash-1.0-off)/flash);
        }

        //draw ball
        draw_ball();
        /*
        strip_leds[ball_p] = color_ball;
        if(prev_ball_p+1 < ball_p)
        {
          for(int f = prev_ball_p+1; f < ball_p; f++)
            strip_leds[f] = dampen_color(color_ball,(float)(f-prev_ball_p)/(prev_ball_p-ball_p));
          if(ball_p+1 <= back(0))
          {
            long amt = (virtual_ball_p*STRIP_NUM_LEDS)%STRIP_NUM_VIRTUAL_LEDS;
            if(amt > STRIP_NUM_VIRTUAL_LEDS/2+1)
            {
              amt = (amt-STRIP_NUM_VIRTUAL_LEDS/2)*2;
              strip_leds[ball_p+1] = dampen_color(color_ball,(float)amt/STRIP_NUM_VIRTUAL_LEDS);
            }
          }
        }
        else if(prev_ball_p-1 > ball_p)
        {
          for(int f = prev_ball_p-1; f > ball_p; f--)
            strip_leds[f] = dampen_color(color_ball,(float)(f-prev_ball_p)/(prev_ball_p-ball_p));
          if(ball_p-1 >= 0)
          {
            long amt = (virtual_ball_p*STRIP_NUM_LEDS)%STRIP_NUM_VIRTUAL_LEDS;
            if(amt < STRIP_NUM_VIRTUAL_LEDS/2)
            {
              amt = STRIP_NUM_VIRTUAL_LEDS-(amt*2);
              strip_leds[ball_p-1] = dampen_color(color_ball,(float)amt/STRIP_NUM_VIRTUAL_LEDS);
            }
          }
        }
        */

      }
        break;
      case STATE_SCORE:
      {
        draw_base_strip();

        if(pong_serve == 1) //a scored
        {
          strip_leds[state_t%zone_a_len] = color_a;
          strip_leds[STRIP_NUM_LEDS-zone_b_len+(state_t%zone_b_len)] = color_a;
          int body_leds = STRIP_NUM_LEDS-zone_a_len-zone_b_len+2;
          int pulse_t = SCORE_T/10;
          int pulse_is = zone_a_len+(( state_t   %pulse_t)*body_leds/pulse_t);
          int pulse_ie = zone_a_len+(((state_t+1)%pulse_t)*body_leds/pulse_t);
          if((state_t+1)%pulse_t == 0) pulse_ie = STRIP_NUM_LEDS-zone_b_len+1;
          if(pulse_is == pulse_ie) pulse_ie++;
          for(int i = pulse_is; i < pulse_ie; i++)
            strip_leds[i] = color_a;
        }
        else if(pong_serve == -1) //b scored
        {
          strip_leds[zone_a_len-1-(state_t%zone_a_len)] = color_b;
          strip_leds[STRIP_NUM_LEDS-1-(state_t%zone_b_len)] = color_b;
          int body_leds = STRIP_NUM_LEDS-zone_a_len-zone_b_len+2;
          int pulse_t = SCORE_T/10;
          int pulse_is = zone_b_len+(( state_t   %pulse_t)*body_leds/pulse_t);
          int pulse_ie = zone_b_len+(((state_t+1)%pulse_t)*body_leds/pulse_t);
          if((state_t+1)%pulse_t == 0) pulse_ie = STRIP_NUM_LEDS-zone_a_len+1;
          if(pulse_is == pulse_ie) pulse_ie++;
          for(int i = pulse_is; i < pulse_ie; i++)
            strip_leds[back(i)] = color_b;
        }

        //draw hits
        if(pong_serve == -1 && btn_a_hit_p != -1) strip_leds[btn_a_hit_p] = color_a;
        if(pong_serve ==  1 && btn_b_hit_p != -1) strip_leds[btn_b_hit_p] = color_b;

        //draw ball
        draw_ball();
        //strip_leds[ball_p] = color_ball;
      }
        break;
      case STATE_DEBUG:
      {
        //draw_base_strip();
        for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;

        draw_ball();
      }
        break;
    }
  }
  strip_ready = 1;
  #ifdef MULTITHREAD
  pthread_mutex_unlock(&strip_lock);
  pthread_cond_signal(&strip_ready_cond);
  #endif

  return 1;
}

void pong_kill()
{
  pong_killed = 1;
  #ifdef MULTITHREAD
  //lie to get myself unstuck
  //no lies necessary!
  #endif
}

void pong_die()
{

}

