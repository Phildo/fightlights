#include "pong.h"
#include <stdlib.h>
#include <pthread.h>
#include "sync.h"

//logic
#define MAX_HIT_ZONE 10 //measured in real LEDs
#define MIN_HIT_ZONE 4  //measured in real LEDs
#define VIRTUAL_LEDS 800

#define SCORE_T 100

int pong_killed;

volatile unsigned char pong_state;
unsigned int state_t;
int speed;
int zone_a_len;
int zone_b_len;
long virtual_ball_p;
long ball_p;
int server;
int serve;
int bounce;
volatile extern unsigned char mio_btn_a_down;
unsigned char btn_a_down;
unsigned int btn_a_down_t;
unsigned int btn_a_press_t;
unsigned int btn_a_up_t;
volatile extern unsigned char mio_btn_b_down;
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
byte color_clear; //dynamic
byte color_a;
byte color_b;
byte color_ball;
byte color_zone;
byte color_blank;
byte color_a_fade[STRIP_FADE_N];
byte color_b_fade[STRIP_FADE_N];
byte color_ball_fade[STRIP_FADE_N];
byte color_red;
byte color_green;

volatile byte strip_leds[STRIP_NUM_LEDS];

//SYNC W/ ARDUINO
void pong_colors_init()
{
  color_blank        = color_clear = 0x0;
  color_ball_fade[0] = color_ball  = 0x1;
  color_ball_fade[1]               = 0x2;
  color_ball_fade[2]               = 0x3;
  color_ball_fade[3]               = 0x4;
  color_a_fade[0]    = color_a     = 0x5;
  color_a_fade[1]                  = 0x6;
  color_a_fade[2]                  = 0x7;
  color_a_fade[3]                  = 0x8;
  color_b_fade[0]    = color_b     = 0x9;
  color_b_fade[1]                  = 0xA;
  color_b_fade[2]                  = 0xB;
  color_b_fade[3]                  = 0xC;
  color_zone                       = 0xD;
  color_red                        = 0xE;
  color_green                      = 0xF;
}

void pong_strip_init()
{
  int i = 0;
  for(; i < MAX_HIT_ZONE; i++)
    strip_leds[i] = color_zone;
  for(; i < STRIP_NUM_LEDS-MAX_HIT_ZONE; i++)
    strip_leds[i] = color_clear;
  for(; i < STRIP_NUM_LEDS; i++)
    strip_leds[i] = color_zone;
}

int back(int i)
{
  return STRIP_NUM_LEDS-1-i;
}

void draw_pulsed_zones()
{
  //a
  {
    int ceilhalf = (zone_a_len+1)/2;
    if(btn_a_press_t && btn_a_press_t < VIRTUAL_STRIP_FADE_N)
    {
      int term_one = VIRTUAL_STRIP_FADE_N*btn_a_press_t;
      int term_two = VIRTUAL_STRIP_FADE_N*ceilhalf;
      for(int i = 0; i < ceilhalf; i++)
      {
        int f = ((VIRTUAL_STRIP_FADE_N-1)-((i*term_one)/term_two)); //\_/
        strip_leds[i]              = color_zone;
        strip_leds[zone_a_len-1-i] = color_zone;
      }
    }
    else
    {
      for(int i = 0; i < ceilhalf; i++)
      {
        int f = ((VIRTUAL_STRIP_FADE_N-1)-((i*VIRTUAL_STRIP_FADE_N)/ceilhalf)); //\_/
        strip_leds[i]              = color_zone;
        strip_leds[zone_a_len-1-i] = color_zone;
      }
    }
    if(btn_a_press_t < zone_a_len)
      strip_leds[btn_a_press_t] = color_a;
  }

  //b
  {
    int ceilhalf = (zone_b_len+1)/2;
    if(btn_b_press_t && btn_b_press_t < VIRTUAL_STRIP_FADE_N)
    {
      int term_one = VIRTUAL_STRIP_FADE_N*btn_b_press_t;
      int term_two = VIRTUAL_STRIP_FADE_N*ceilhalf;
      for(int i = 0; i < ceilhalf; i++)
      {
        int f = ((VIRTUAL_STRIP_FADE_N-1)-((i*term_one)/term_two)); //\_/
        strip_leds[back(i)]              = color_zone;
        strip_leds[back(zone_b_len-1-i)] = color_zone;
      }
    }
    else
    {
      for(int i = 0; i < ceilhalf; i++)
      {
        int f = ((VIRTUAL_STRIP_FADE_N-1)-((i*VIRTUAL_STRIP_FADE_N)/ceilhalf)); //\_/
        strip_leds[back(i)]              = color_zone;
        strip_leds[back(zone_b_len-1-i)] = color_zone;
      }
    }
    if(btn_b_press_t < zone_b_len)
      strip_leds[back(btn_b_press_t)] = color_b;
  }
}

void draw_pulsed_lane()
{
  //a
  {
    if(btn_a_press_t && btn_a_press_t < VIRTUAL_STRIP_FADE_N)
    {
      int len = (STRIP_NUM_LEDS/2)-zone_a_len;
      int term_one = (VIRTUAL_STRIP_FADE_N-1)*(VIRTUAL_STRIP_FADE_N-btn_a_press_t);
      int term_two = len*VIRTUAL_STRIP_FADE_N;
      for(int i = 0; i < len; i++)
      {
        //((1-(i/len))*(1-(btn_a_press_t/VIRTUAL_STRIP_FADE_N)))*(VIRTUAL_STRIP_FADE_N-1) //gets algebra'd into:
        //((len-i)*(VIRTUAL_STRIP_FADE_N-1)*(VIRTUAL_STRIP_FADE_N-btn_a_press_t))/(len*VIRTUAL_STRIP_FADE_N)
        //int f = (((len-i)*term_one)/term_two); //<- would be above eqn
        int f = (((len-i)*term_one)/term_two)-btn_a_press_t; //<- but I instead modified this, so it isn't _any_ of the eqns above
        if(f >= 0) strip_leds[zone_a_len+i] = color_a_fade[f/VIRTUAL_STRIP_FADE_N_MUL];
        else       strip_leds[zone_a_len+i] = color_clear;
      }
    }
    else //just clear lane
    {
      int half = STRIP_NUM_LEDS/2;
      for(int i = zone_a_len; i < half; i++)
        strip_leds[i] = color_clear;
    }
  }

  //b
  {
    if(btn_b_press_t && btn_b_press_t < VIRTUAL_STRIP_FADE_N)
    {
      int len = (STRIP_NUM_LEDS/2)-zone_b_len;
      int term_one = (VIRTUAL_STRIP_FADE_N-1)*(VIRTUAL_STRIP_FADE_N-btn_b_press_t);
      int term_two = len*VIRTUAL_STRIP_FADE_N;
      for(int i = 0; i < len; i++)
      {
        //((1-(i/len))*(1-(btn_b_press_t/VIRTUAL_STRIP_FADE_N)))*(VIRTUAL_STRIP_FADE_N-1) //gets algebra'd into:
        //((len-i)*(VIRTUAL_STRIP_FADE_N-1)*(VIRTUAL_STRIP_FADE_N-btn_b_press_t))/(len*VIRTUAL_STRIP_FADE_N)
        //int f = (((len-i)*term_one)/term_two); //<- would be above eqn
        int f = (((len-i)*term_one)/term_two)-btn_b_press_t; //<- but I instead modified this, so it isn't _any_ of the eqns above
        if(f >= 0) strip_leds[back(zone_b_len+i)] = color_b_fade[f/VIRTUAL_STRIP_FADE_N_MUL];
        else       strip_leds[back(zone_b_len+i)] = color_clear;
      }
    }
    else //just clear lane
    {
      int tip = back(zone_b_len);
      for(int i = STRIP_NUM_LEDS/2; i <= tip; i++)
        strip_leds[i] = color_clear;
    }
  }
}

void clear_lane()
{
  int imax = back(zone_b_len);
  for(int i = zone_a_len; i <= imax; i++)
    strip_leds[i] = color_clear;
}

void set_state(int s);
void pong_init()
{
  //strip
  pong_colors_init();
  pong_strip_init();

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

  set_state(STATE_SIGNUP);

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
      speed = 3;
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
        ball_p = 0;
      }
      else
      {
        server = -1;
        virtual_ball_p = VIRTUAL_LEDS-1;
        ball_p = back(0);
      }
      serve = server;
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

  btn_a_down = mio_btn_a_down;
  btn_b_down = mio_btn_b_down;

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
      virtual_ball_p += serve*speed;
      int old_ball_p = ball_p;
      ball_p = virtual_ball_p*STRIP_NUM_LEDS/VIRTUAL_LEDS;
      if(ball_p >= STRIP_NUM_LEDS) { ball_p = back(0); set_state(STATE_SCORE); break; }
      else if(ball_p < 0)          { ball_p = 0;       set_state(STATE_SCORE); break; }

      //clear hits at midpoint
      int midpoint = STRIP_NUM_LEDS/2;
           if(serve ==  1 && old_ball_p < midpoint && ball_p >= midpoint) { btn_b_hit_p = -1; missile_b_hit_p = -1; }
      else if(serve == -1 && old_ball_p > midpoint && ball_p <= midpoint) { btn_a_hit_p = -1; missile_a_hit_p = -1; }

      //handle hits
      int should_bounce = 0;
      if(btn_a_hit_p == -1 && btn_a_down_t == 1) btn_a_hit_p = ball_p;
      if(btn_b_hit_p == -1 && btn_b_down_t == 1) btn_b_hit_p = ball_p;

      if(serve == -1 && btn_a_press_t < zone_a_len && ball_p <= btn_a_press_t)
      {
        missile_a_hit_p = ball_p;
        missile_a_hit_t = 1;
        should_bounce = 1;
      }
      if(serve == 1 && btn_b_press_t < zone_b_len && ball_p >= back(btn_b_press_t))
      {
        missile_b_hit_p = ball_p;
        missile_b_hit_t = 1;
        should_bounce = 1;
      }

      if(should_bounce)
      {
        bounce++;
        speed = 3+(bounce/3);
             if(serve == -1) { serve =  1; zone_a_len = MAX_HIT_ZONE-((bounce+2)/3); } //a served
        else if(serve ==  1) { serve = -1; zone_b_len = MAX_HIT_ZONE-((bounce+2)/3); } //b served
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
  color_clear = color_blank;
  //for(int i = 0; i < 10; i++) //used to test performance
  {
    switch(pong_state)
    {
      case STATE_SIGNUP:
      {
        draw_pulsed_zones();
        draw_pulsed_lane();

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
        if(t && t < VIRTUAL_STRIP_FADE_N) color_clear = color_ball_fade[(VIRTUAL_STRIP_FADE_N-t)/VIRTUAL_STRIP_FADE_N_MUL];

        draw_pulsed_zones();
        draw_pulsed_lane();
        if(bounce%3)
        {
          if(serve == 1 && missile_a_hit_t < VIRTUAL_STRIP_FADE_N*2)
          {
            if(missile_a_hit_t > VIRTUAL_STRIP_FADE_N && (missile_a_hit_t/2)%2) strip_leds[zone_a_len] = color_red;
            else                                                                strip_leds[zone_a_len] = color_zone;
          }
          if(serve == -1 && missile_b_hit_t < VIRTUAL_STRIP_FADE_N*2)
          {
            if(missile_b_hit_t > VIRTUAL_STRIP_FADE_N && (missile_b_hit_t/2)%2) strip_leds[back(zone_b_len)] = color_red;
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
          if(missile_a_hit_p-off >= 0)              strip_leds[missile_a_hit_p-off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
          if(missile_a_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_a_hit_p+off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
        }
        off = missile_b_hit_t/2;
        if(missile_b_hit_t && off < VIRTUAL_STRIP_FADE_N*3)
        {
          off = off%VIRTUAL_STRIP_FADE_N;
          if(missile_b_hit_p-off >= 0)              strip_leds[missile_b_hit_p-off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
          if(missile_b_hit_p+off <  STRIP_NUM_LEDS) strip_leds[missile_b_hit_p+off] = color_ball_fade[(VIRTUAL_STRIP_FADE_N-1-off)/VIRTUAL_STRIP_FADE_N_MUL];
        }

        //draw ball
        strip_leds[ball_p] = color_ball;
      }
        break;
      case STATE_SCORE:
      {
        draw_pulsed_zones();
        clear_lane();

        if(serve == 1) //a scored
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
        else if(serve == -1) //b scored
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
        if(serve == -1 && btn_a_hit_p != -1) strip_leds[btn_a_hit_p] = color_a;
        if(serve ==  1 && btn_b_hit_p != -1) strip_leds[btn_b_hit_p] = color_b;

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

