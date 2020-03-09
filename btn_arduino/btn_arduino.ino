#define NOMIDDLEMAN
#define NOSTRIP
//#define NORGB
#define NOBUZZER
#define NOMIC
//#define NOPOT

#ifndef NOMIDDLEMAN
#include <NeoSWSerial.h>
#endif

#ifndef NOSTRIP
#include <FastLED.h>
#endif

//customize
#define PLAYER 1 //0 or 1
#define STRIP_BRIGHTNESS 255 //255 //0-255
#define BUZZER_MAX 300
#define LED_UPDATE_T_MAX 10 //set too low and we drop serial input
#define SIGNUP_T_MAX 300
#define PLAY_T_MAX 300
#define SCORE_T_MAX 300
#define MIC_HOT_STAY 100

//serial constants (sync w/ pi)
#define BAUD_RATE 1000000
#if PLAYER == 0
#define AID "BTN0\n"
#else
#define AID "BTN1\n"
#endif
#define CMD_PREAMBLE "CMD_"
#define T_CONSIDERED_DEAD 10000

#ifndef NOMIDDLEMAN
//soft serial constants (sinc w/ mio)
#define SOFT_BAUD_RATE 9600
#endif

//enum
#define CMD_WHORU '0'
#define CMD_DATA '1'
#define CMD_SYNC '2'

//player constants
#define PLAYER_0_R 0xFF
#define PLAYER_0_G 0x00
#define PLAYER_0_B 0x00
#define PLAYER_0_SPIN -1
#define PLAYER_1_R 0x00
#define PLAYER_1_G 0x00
#define PLAYER_1_B 0xFF
#define PLAYER_1_SPIN 1

//player adopt
#if PLAYER == 0
  #define PLAYER_R PLAYER_0_R
  #define PLAYER_G PLAYER_0_G
  #define PLAYER_B PLAYER_0_B
  #define PLAYER_SPIN PLAYER_0_SPIN
  #define OPPO_R PLAYER_1_R
  #define OPPO_G PLAYER_1_G
  #define OPPO_B PLAYER_1_B
  #define OPPO_SPIN PLAYER_1_SPIN
#else
  #define PLAYER_R PLAYER_1_R
  #define PLAYER_G PLAYER_1_G
  #define PLAYER_B PLAYER_1_B
  #define PLAYER_SPIN PLAYER_1_SPIN
  #define OPPO_R PLAYER_0_R
  #define OPPO_G PLAYER_0_G
  #define OPPO_B PLAYER_0_B
  #define OPPO_SPIN PLAYER_0_SPIN
#endif

//strip constants
#define STRIP_NUM_LEDS 16
#define STRIP_NUM_VIRTUAL_PER_LED 100
#define STRIP_NUM_VIRTUAL_LEDS (STRIP_NUM_LEDS*STRIP_NUM_VIRTUAL_PER_LED)
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER GBR

//pins
// in
#define MIC_PIN 2
#define BTN_PIN 3
#define POT_PIN A6
#ifndef NOMIDDLEMAN
#define MIO_RX_PIN 5
#define MIO_TX_PIN 4
#endif
// out
#define IO_PIN 8
#define LED_PIN 9
#define STRIP_LED_PIN 10
#define SPEAKER_PIN 11
//CANNOT USE RGB LED WITH STRIP/BUZZER! (only issue is confliciting pins- shuffling pins might resolve it)
#define RGB_R_PIN 9
#define RGB_G_PIN 10
#define RGB_B_PIN 11

//modes
#define MODE_SIGNUP 0
#define MODE_PLAY 1
#define MODE_SCORE 2
#define MODE_DATA_ME 0
#define MODE_DATA_THEM 1

#define MODE_T_MAX 1000

//visparams
#define MODE_SIGNUP_SPEED 2
#define MODE_PLAY_SPEED 10
#define MODE_SCORE_SPEED 30

#ifndef NOSTRIP
//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB clear;
#endif

#ifndef NOMIDDLEMAN
//softser
NeoSWSerial mio_ser(MIO_RX_PIN,MIO_TX_PIN);
#endif

//state
unsigned char mode;
unsigned int mode_t;
unsigned char mode_data;
unsigned int mode_data_t;
unsigned int strip_state;
unsigned int rgb_state;
//btn
unsigned char btn_down;
int since_btn_down_t;
int btn_down_buzz;
#ifndef NOMIC
//mic
unsigned int mic_hot;
volatile unsigned int im_mic_hot;
void mic_interrupt() { im_mic_hot = MIC_HOT_STAY; }
#endif
unsigned int pot_amt; //normalized between 0-10
//led
unsigned char led_update_t;

unsigned char serial_spinread(char *c)
{
  unsigned int cmd_dead_t = 0;
  while(cmd_dead_t < T_CONSIDERED_DEAD)
  {
    if(Serial.available()) { *c = Serial.read(); return 1; }
    cmd_dead_t++;
  }
  return 0;
}

void set_mode_from_char(char d)
{
  unsigned char new_mode = 0;
  unsigned char new_mode_data = 0;
  switch(d & 0x3) //00000011
  {
    case 0: new_mode = MODE_SIGNUP; break;
    case 1: new_mode = MODE_PLAY; break;
    case 2: new_mode = MODE_SCORE; break;
  }
  switch((d & 0x7) >> 2) //00000111 >> 2
  {
    case 0: new_mode_data = MODE_DATA_ME; break;
    case 1: new_mode_data = MODE_DATA_THEM; break;
  }
  if(new_mode != mode) mode_t = 0;
  else if(new_mode_data != mode_data) mode_data_t = 0;
  mode = new_mode;
  mode_data = new_mode_data;
}

void cmd_data()
{
  char d;
  if(!serial_spinread(&d)) return;
  set_mode_from_char(d);
  Serial.write(btn_down);//good idea to update button status on any delta
}

void cmd_sync()
{
  Serial.write(btn_down);
  Serial.write(pot_amt << 1);
}

void cmd_whoru()
{
  Serial.write(AID);
}

void cmd_loop()
{
  char d;

  if(Serial.available())
  {
    d = Serial.read();
    if(d == CMD_PREAMBLE[0])
    {
      unsigned char i = 1;
      while(serial_spinread(&d) && d == CMD_PREAMBLE[i] && i < strlen(CMD_PREAMBLE)) i++; //note- will read one extra character (perfect!)
      if(i == strlen(CMD_PREAMBLE))
      {
        switch(d)
        {
          case CMD_WHORU: cmd_whoru(); break;
          case CMD_DATA:  cmd_data();  break;
          case CMD_SYNC:  cmd_sync();  break;
        }
      }
    }
  }
}

#ifndef NOSTRIP
void tickdraw_strip(unsigned long t_r, unsigned long t_g, unsigned long t_b, int speed)
{
  unsigned int target_i = strip_state/STRIP_NUM_VIRTUAL_PER_LED;
  unsigned long shade   = strip_state%STRIP_NUM_VIRTUAL_PER_LED;
  unsigned int off_i = (target_i+(STRIP_NUM_LEDS-1))%STRIP_NUM_LEDS;

  unsigned long r;
  unsigned long g;
  unsigned long b;
  strip_state = (strip_state+STRIP_NUM_VIRTUAL_LEDS+speed)%STRIP_NUM_VIRTUAL_LEDS;

  r = shade*t_r/STRIP_NUM_VIRTUAL_PER_LED;
  g = shade*t_g/STRIP_NUM_VIRTUAL_PER_LED;
  b = shade*t_b/STRIP_NUM_VIRTUAL_PER_LED;
  unsigned long target_color = (unsigned long)(r << 16) | (unsigned long)(g << 8) | b;
  shade = STRIP_NUM_VIRTUAL_PER_LED-shade-1;
  r = shade*t_r/STRIP_NUM_VIRTUAL_PER_LED;
  g = shade*t_g/STRIP_NUM_VIRTUAL_PER_LED;
  b = shade*t_b/STRIP_NUM_VIRTUAL_PER_LED;
  unsigned long off_color = (unsigned long)(r << 16) | (unsigned long)(g << 8) | b;
  strip_leds[target_i] = target_color;
  strip_leds[off_i] = off_color;
  led_update_t = (led_update_t+1)%LED_UPDATE_T_MAX;
  if(led_update_t == 0) FastLED.show();
  strip_leds[target_i] = 0x000000;
  strip_leds[off_i]    = 0x000000;
}
#endif

#ifndef NORGB
void tickdraw_rgb(unsigned long t_r, unsigned long t_g, unsigned long t_b, int speed) //this is overly complex bc I just quickly copied tickdraw_strip- can be greatly simplified
{
  unsigned int slow = 1; //modifier on strip logic
  unsigned long shade = (rgb_state%(STRIP_NUM_VIRTUAL_PER_LED*slow))/slow;

  unsigned long r;
  unsigned long g;
  unsigned long b;
  int n = STRIP_NUM_VIRTUAL_LEDS*slow;
  rgb_state = (rgb_state+n+speed)%n;

  if(rgb_state < STRIP_NUM_VIRTUAL_LEDS*slow/2)
  {
    unsigned long shade = rgb_state*2;
    r = shade*t_r/n;
    g = shade*t_g/n;
    b = shade*t_b/n;
  }
  else
  {
    unsigned long shade = (n-rgb_state)*2;
    r = shade*t_r/n;
    g = shade*t_g/n;
    b = shade*t_b/n;
  }

  led_update_t = (led_update_t+1)%LED_UPDATE_T_MAX;
  if(led_update_t == 0)
  {
    analogWrite(RGB_R_PIN,r);
    analogWrite(RGB_G_PIN,g);
    analogWrite(RGB_B_PIN,b);
  }
}
#endif

void setup()
{
  delay(300); //power-up safety delay

  //kill debug LED
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

  //init serial
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  #ifndef NOMIDDLEMAN
  //softserial already init- begin
  mio_ser.begin(SOFT_BAUD_RATE);
  #endif

  //out
  pinMode(LED_PIN,OUTPUT);
  #ifndef NOBUZZER
  pinMode(SPEAKER_PIN,OUTPUT);
  #endif
  #ifndef NORGB
  pinMode(RGB_R_PIN,OUTPUT);
  pinMode(RGB_G_PIN,OUTPUT);
  pinMode(RGB_B_PIN,OUTPUT);
  #endif
  pinMode(IO_PIN,OUTPUT);
  digitalWrite(IO_PIN,LOW);
  //in
  #ifndef NOMIC
  pinMode(MIC_PIN,INPUT);
  #endif
  pinMode(BTN_PIN,INPUT_PULLUP);
  #if !defined NOPOT && PLAYER == 0
  pinMode(POT_PIN,INPUT);
  #endif

  /*
  //mic
  //  apparently RPi (compilation src) ships with very old version of arduino, and
  //  "digitalPinToInterrupt" is erroneously missing in that version. I don't
  //  want to mess with this compilation pipeline I don't understand, so am doing
  //  the "unrecommended" thing of just doing the translation directly
  //attachInterrupt(digitalPinToInterrupt(MIC_PIN), mic_interrupt, RISING);
  attachInterrupt(0, mic_interrupt, RISING);
  mic_hot = 0;
  im_mic_hot = 0;
  */

  #ifndef NOSTRIP
  //init strip
  clear = CRGB(0x00,0x00,0x00);
  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;
  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  FastLED.setDither(0);
  FastLED.show();
  #endif

  digitalWrite(LED_PIN,LOW);

  //init state
  strip_state = 0;
  btn_down = 0;
  since_btn_down_t = 0;
  btn_down_buzz = 0;
  led_update_t = 0;
  mode = MODE_SIGNUP;
  mode_t = 0;
  mode_data = MODE_DATA_ME;
  mode_data_t = 0;
}

void loop()
{
  #ifndef NOMIC
  mic_hot = im_mic_hot;
  if(im_mic_hot) im_mic_hot--;
  #endif

  #if !defined NOPOT && PLAYER == 0
  unsigned int new_pot_amt = analogRead(POT_PIN)*11/1024;
  if(new_pot_amt > 10) new_pot_amt = 10;
  if(new_pot_amt < 1)  new_pot_amt = 1;
  if(pot_amt != new_pot_amt)
    Serial.write(new_pot_amt << 1);
  pot_amt = new_pot_amt;
  #endif

  char btn_delta = 0;
  if(!digitalRead(BTN_PIN)) { if(!btn_down) { btn_delta =  1; btn_down_buzz = BUZZER_MAX/10; since_btn_down_t = 0; } btn_down = 1; btn_down_buzz++;  if(btn_down_buzz > BUZZER_MAX) btn_down_buzz = BUZZER_MAX*9/10; }
  else                      { if( btn_down) { btn_delta = -1;                                                      } btn_down = 0; btn_down_buzz-=3; if(btn_down_buzz < 0         ) btn_down_buzz = 0;               }

  if(btn_delta)
  {
    if(btn_down) digitalWrite(IO_PIN,HIGH);
    else         digitalWrite(IO_PIN,LOW);
    #ifdef NOMIDDLEMAN
    Serial.write(btn_down);
    #else
    for(int i = 0; i < 10; i++) //idempotent state, so just make sure it gets sent
    {
      mio_ser.write(btn_down);
    }
    #endif
  }

  #ifdef NOMIDDLEMAN
  //delegate to cmd loop!
  #else
  while(mio_ser.available())
    set_mode_from_char(mio_ser.read());
  #endif

  switch(mode)
  {
    case MODE_SIGNUP:
    {
      #ifndef NOBUZZER
      //speaker
      if(btn_down_buzz) tone(SPEAKER_PIN,500+btn_down_buzz*10);
      else noTone(SPEAKER_PIN);
      #endif

      #ifndef NOSTRIP
      if(!btn_down) tickdraw_strip(PLAYER_R,PLAYER_G,PLAYER_B, MODE_SIGNUP_SPEED);
      else          tickdraw_strip(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_SIGNUP_SPEED);
      #endif
      #ifndef NORGB
      if(!btn_down) tickdraw_rgb(PLAYER_R,PLAYER_G,PLAYER_B, MODE_SIGNUP_SPEED);
      else          tickdraw_rgb(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_SIGNUP_SPEED);
      #endif
    }
    break;
    case MODE_PLAY:
    {
      #ifndef NOBUZZER
      if(mode_t < BUZZER_MAX) //play begins- launch
      {
        switch(mode_t%3)
        {
          case 0: tone(SPEAKER_PIN,1000+BUZZER_MAX*10-mode_t*10); break;
          case 1: tone(SPEAKER_PIN, 800+BUZZER_MAX*10-mode_t*10); break;
          case 2: tone(SPEAKER_PIN, 500+BUZZER_MAX*10-mode_t*10); break;
        }
      }
      else if(mode_data == MODE_DATA_ME && mode_data_t < BUZZER_MAX) //newly bounced ball
        tone(SPEAKER_PIN,1000+BUZZER_MAX*10-mode_data_t*20);
      else if(since_btn_down_t < BUZZER_MAX/5) //button pressed- could be hit/miss!
        tone(SPEAKER_PIN,1000+since_btn_down_t*20);
      else noTone(SPEAKER_PIN);
      #endif

      #ifndef NOSTRIP
      if(!btn_down) tickdraw_strip(PLAYER_R,PLAYER_G,PLAYER_B, MODE_PLAY_SPEED);
      else          tickdraw_strip(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_PLAY_SPEED);
      #endif
      #ifndef NORGB
      if(!btn_down) tickdraw_rgb(PLAYER_R,PLAYER_G,PLAYER_B, MODE_PLAY_SPEED);
      else          tickdraw_rgb(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_PLAY_SPEED);
      #endif
    }
    break;
    case MODE_SCORE:
    {
      #ifndef NOBUZZER
      //speaker
      if(mode_data == MODE_DATA_ME) tone(SPEAKER_PIN,500+              ((mode_t*4)%BUZZER_MAX)*10);
      else                          tone(SPEAKER_PIN,500+BUZZER_MAX*10-((mode_t*4)%BUZZER_MAX)*10);
      #endif

      #ifndef NOSTRIP
      //TODO: should be "who won", not "btn down"
      if(!btn_down) tickdraw_strip(PLAYER_R,PLAYER_G,PLAYER_B, MODE_SCORE_SPEED);
      else          tickdraw_strip(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_SCORE_SPEED);
      #endif
      #ifndef NORGB
      if(!btn_down) tickdraw_rgb(PLAYER_R,PLAYER_G,PLAYER_B, MODE_SCORE_SPEED);
      else          tickdraw_rgb(  OPPO_R,  OPPO_G,  OPPO_B,-MODE_SCORE_SPEED);
      #endif
    }
    break;
  }
  delay(3);

  cmd_loop();

  mode_t++;           if(mode_t      > MODE_T_MAX) mode_t      = MODE_T_MAX;
  mode_data_t++;      if(mode_data_t > MODE_T_MAX) mode_data_t = MODE_T_MAX;
  since_btn_down_t++; if(since_btn_down_t > MODE_T_MAX) since_btn_down_t = MODE_T_MAX;
}

