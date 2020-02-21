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
#define STRIP_NUM_VIRTUAL_PER_LED 4
#define STRIP_NUM_VIRTUAL_LEDS (STRIP_NUM_LEDS*STRIP_NUM_VIRTUAL_PER_LED)

#define SCORE_T 100

int pong_killed;

unsigned int t_mod_twelvepi_100;
volatile unsigned char pong_state;
unsigned int state_t;
int speed;
int zone_a_len;
int zone_b_len;
long prev_virtual_ball_p;
long virtual_ball_p;
long prev_ball_p;
long ball_p;
volatile int pong_serve;
int server;
int bounce;
volatile extern unsigned char mio_btn_down[2];
unsigned char btn_a_down;
unsigned int btn_a_down_t;
unsigned int btn_a_press_t;
unsigned int btn_a_up_t;
unsigned char btn_b_down;
unsigned int btn_b_down_t;
unsigned int btn_b_press_t;
unsigned int btn_b_up_t;
int btn_a_hit_p;
int missile_a_hit_p;
unsigned int missile_a_hit_t;
int btn_b_hit_p;
int missile_b_hit_p;
unsigned int missile_b_hit_t;

#define STRIP_FADE_N 4
#define VIRTUAL_STRIP_FADE_N_MUL 4
#define VIRTUAL_STRIP_FADE_N (STRIP_FADE_N*VIRTUAL_STRIP_FADE_N_MUL)

color clear;
color black;
color white;
color red;
color green;
color blue;
color yellow;
color light_blue;
color dark_blue;
color pink;

color color_a;
color color_b;
color color_ball;
color color_zone;

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

void draw_base_strip()
{
  int i = 0;
  color c = dampen_color(color_zone,0.01);
  for(; i < zone_a_len; i++)
  {
    float d = 0.5+sin(100.0-t_mod_twelvepi_100/10.0f+i/2.0)/2.0;
    strip_leds[i] = dampen_color(color_zone,d*d*d);
    strip_leds[i] = dampen_color(strip_leds[i],0.1);
  }
  for(; i < STRIP_NUM_LEDS-zone_b_len; i++)
    strip_leds[i] = clear;
  for(; i < STRIP_NUM_LEDS; i++)
  {
    float d = 0.5+sin(t_mod_twelvepi_100/10.0f+i/2.0)/2.0;
    strip_leds[i] = dampen_color(color_zone,d*d*d);
    strip_leds[i] = dampen_color(strip_leds[i],0.1);
  }
}

void clear_lane()
{
  int imax = back(zone_b_len);
  for(int i = zone_a_len; i <= imax; i++)
    strip_leds[i] = clear;
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

  t_mod_twelvepi_100 = 0;

  set_state(STATE_SIGNUP);
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
      speed = 1;
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
      if(t >= STRIP_NUM_LEDS/2) set_state(STATE_PLAY);
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
        bounce++;
        speed = 1+(bounce/6);
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
        if(btn_a_down_t || btn_b_down_t) //at least one down
        {
          //pulse
          if(btn_a_down_t >= btn_b_down_t) //a first
          {
            strip_leds[                             (btn_a_down_t+MAX_HIT_ZONE-1)%MAX_HIT_ZONE ] = color_a;
            strip_leds[STRIP_NUM_LEDS-MAX_HIT_ZONE+((btn_a_down_t+MAX_HIT_ZONE-1)%MAX_HIT_ZONE)] = color_a;
          }
          else //b first
          {
            strip_leds[  MAX_HIT_ZONE-1- (btn_b_down_t+MAX_HIT_ZONE-1)%MAX_HIT_ZONE ] = color_b;
            strip_leds[STRIP_NUM_LEDS-1-((btn_b_down_t+MAX_HIT_ZONE-1)%MAX_HIT_ZONE)] = color_b;
          }

          //static
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

          //fill zone
          if(btn_a_down_t && btn_b_down_t) //both buttons down
          {
            int t = btn_a_down_t;
            if(btn_b_down_t < btn_a_down_t) t = btn_b_down_t;
            for(int i = MAX_HIT_ZONE+1; i <= t; i++)
            {
              strip_leds[i] = color_a;
              strip_leds[back(i)] = color_b;
            }
          }

        }
      }
        break;
      case STATE_PLAY:
      {
        int t = missile_a_hit_t;
        if(!missile_a_hit_t || (missile_a_hit_t && missile_b_hit_t && missile_a_hit_t > missile_b_hit_t)) t = missile_b_hit_t; //t == the lowest non-zero missile_[ab]_hit_t, or 0
        //if(t && t < VIRTUAL_STRIP_FADE_N) clear = color_ball_fade[(VIRTUAL_STRIP_FADE_N-t)/VIRTUAL_STRIP_FADE_N_MUL];

        if(bounce%3)
        {
          if(pong_serve == 1 && missile_a_hit_t < VIRTUAL_STRIP_FADE_N*2)
          {
            if(missile_a_hit_t > VIRTUAL_STRIP_FADE_N && (missile_a_hit_t/2)%2) strip_leds[zone_a_len] = red;
            else                                                                strip_leds[zone_a_len] = color_zone;
          }
          if(pong_serve == -1 && missile_b_hit_t < VIRTUAL_STRIP_FADE_N*2)
          {
            if(missile_b_hit_t > VIRTUAL_STRIP_FADE_N && (missile_b_hit_t/2)%2) strip_leds[back(zone_b_len)] = red;
            else                                                                strip_leds[back(zone_b_len)] = color_zone;
          }
        }

        //draw hits
        if(missile_a_hit_p != -1) strip_leds[missile_a_hit_p] = color_a;
        if(missile_b_hit_p != -1) strip_leds[missile_b_hit_p] = color_b;

        //particles
        int off;
        off = missile_a_hit_t/2;
        if(missile_a_hit_t && off < VIRTUAL_STRIP_FADE_N*3)
        {
          off = off%VIRTUAL_STRIP_FADE_N;
          //if(missile_a_hit_p-off >= 0)              strip_leds[missile_a_hit_p-off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
          //if(missile_a_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_a_hit_p+off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
        }
        off = missile_b_hit_t/2;
        if(missile_b_hit_t && off < VIRTUAL_STRIP_FADE_N*3)
        {
          off = off%VIRTUAL_STRIP_FADE_N;
          //if(missile_b_hit_p-off >= 0)              strip_leds[missile_b_hit_p-off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
          //if(missile_b_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_b_hit_p+off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
        }

        //draw ball
        strip_leds[ball_p] = color_ball;
        if(prev_ball_p+1 < ball_p)
        {
          //for(int f = prev_ball_p+1; f < ball_p; f++)
            //strip_leds[f] = color_ball_fade[3+(4*(f-prev_ball_p)/(prev_ball_p-ball_p))];
          if(ball_p+1 <= back(0))
          {
            long amt = (virtual_ball_p*STRIP_NUM_LEDS)%STRIP_NUM_VIRTUAL_LEDS;
            if(amt > STRIP_NUM_VIRTUAL_LEDS/2+1)
            {
              amt = (amt-STRIP_NUM_VIRTUAL_LEDS/2)*2;
              //strip_leds[ball_p+1] = color_ball_fade[5*amt/STRIP_NUM_VIRTUAL_LEDS];
            }
          }
        }
        else if(prev_ball_p-1 > ball_p)
        {
          //for(int f = prev_ball_p-1; f > ball_p; f--)
            //strip_leds[f] = color_ball_fade[3+(4*(f-prev_ball_p)/(prev_ball_p-ball_p))];
          if(ball_p-1 >= 0)
          {
            long amt = (virtual_ball_p*STRIP_NUM_LEDS)%STRIP_NUM_VIRTUAL_LEDS;
            if(amt < STRIP_NUM_VIRTUAL_LEDS/2)
            {
              amt = STRIP_NUM_VIRTUAL_LEDS-(amt*2);
              //strip_leds[ball_p-1] = color_ball_fade[5*amt/STRIP_NUM_VIRTUAL_LEDS];
            }
          }
        }

      }
        break;
      case STATE_SCORE:
      {
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
        strip_leds[ball_p] = color_ball;
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

