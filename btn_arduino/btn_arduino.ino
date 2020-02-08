#include <FastLED.h>
#include <NeoSWSerial.h>

//customize
#define PLAYER 0 //0 or 1
#define STRIP_BRIGHTNESS 120 //255 //0-255
#define BUZZER_MAX 300
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

//soft serial constants (sinc w/ mio)
#define SOFT_BAUD_RATE 9600

//enum
#define CMD_WHORU '0'
#define CMD_DATA '1'

//player constants
#define PLAYER_0_R 0xFF
#define PLAYER_0_G 0x00
#define PLAYER_0_B 0x00
#define PLAYER_0_SPIN -1
#define PLAYER_1_R 0x00
#define PLAYER_1_G 0xFF
#define PLAYER_1_B 0x00
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
#define STRIP_NUM_VIRTUAL_PER_LED 10
#define STRIP_NUM_VIRTUAL_LEDS (STRIP_NUM_LEDS*STRIP_NUM_VIRTUAL_PER_LED)
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER GRB

//pins
// in
#define MIC_PIN 2
#define BTN_PIN 3
#define MIO_RX_PIN 5
#define MIO_TX_PIN 4
// out
#define IO_PIN 8
#define LED_PIN 9
#define STRIP_LED_PIN 10
#define SPEAKER_PIN 11

//modes
#define MODE_SIGNUP 0
#define MODE_PLAY 1
#define MODE_SCORE 2
#define MODE_DATA_ME 0
#define MODE_DATA_THEM 1

#define MODE_T_MAX 1000

//visparams
#define MODE_SIGNUP_SPEED 1
#define MODE_PLAY_SPEED 3
#define MODE_SCORE_SPEED 10

//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB clear;

//softser
NeoSWSerial mio_ser(MIO_RX_PIN,MIO_TX_PIN);

//state
unsigned char mode;
unsigned int mode_t;
unsigned char mode_data;
unsigned char io_transitioning;
unsigned int ring_state;
//btn
unsigned char btn_down;
int btn_down_t;
//mic
unsigned int mic_hot;
volatile unsigned int im_mic_hot;
void mic_interrupt() { im_mic_hot = MIC_HOT_STAY; }

//cmdloop
unsigned char cmd_trigger_i;

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

void cmd_data()
{
  //do nothing
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
    if(d == CMD_PREAMBLE[cmd_trigger_i])
    {
      cmd_trigger_i++;
      if(cmd_trigger_i == strlen(CMD_PREAMBLE))
      {
        if(!serial_spinread(&d)) { cmd_trigger_i = 0; return; }
        switch(d)
        {
          case CMD_WHORU: cmd_whoru(); break;
          case CMD_DATA:  cmd_data(); break;
        }
        cmd_trigger_i = 0;
      }
    }
    else cmd_trigger_i = 0;
  }
}

void setup()
{
  delay(300); //power-up safety delay

  //init serial
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  //softserial already init- begin
  mio_ser.begin(SOFT_BAUD_RATE);

  //out
  //pinMode(STRIP_LED_PIN,OUTPUT); //defer to FastLED
  pinMode(LED_PIN,OUTPUT);
  pinMode(SPEAKER_PIN,OUTPUT);
  pinMode(IO_PIN,OUTPUT);
  digitalWrite(IO_PIN,LOW);
  //in
  pinMode(MIC_PIN,INPUT);
  pinMode(BTN_PIN,INPUT_PULLUP);

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

  //init strip
  clear = CRGB(0x00,0x00,0x00);
  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;
  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  FastLED.show();

  digitalWrite(LED_PIN,LOW);
  digitalWrite(13,LOW); //on-board led

  //init state
  ring_state = 0;
  btn_down = 0;
  btn_down_t = 0;
  mode = MODE_SIGNUP;
  mode_data = MODE_DATA_ME;
  io_transitioning = 0;
}

void loop()
{
  mic_hot = im_mic_hot;
  if(im_mic_hot) im_mic_hot--;

  char btn_delta = 0;
  if(!digitalRead(BTN_PIN)) { if(!btn_down) { btn_delta =  1; btn_down_t = BUZZER_MAX/10; } btn_down = 1; btn_down_t++;  if(btn_down_t > BUZZER_MAX) btn_down_t = BUZZER_MAX*9/10; }
  else                      { if( btn_down) { btn_delta = -1;                             } btn_down = 0; btn_down_t-=3; if(btn_down_t < 0         ) btn_down_t = 0;               }

  if(btn_delta)
  {
    if(btn_down) digitalWrite(IO_PIN,HIGH);
    else         digitalWrite(IO_PIN,LOW);
    for(int i = 0; i < 10; i++) //idempotent state, so just make sure it gets sent
    {
      mio_ser.write(btn_down);
    }
  }

  while(mio_ser.available())
  {
    unsigned char new_mode = 0;
    unsigned char new_mode_data = 0;
    char d = mio_ser.read();
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
    if(new_mode != mode || new_mode_data != mode_data) mode_t = 0;
  }
  mode_t++;
  if(mode_t > MODE_T_MAX) mode_t = MODE_T_MAX;

  switch(mode)
  {
    case MODE_SIGNUP:
    {
      //speaker
      if(btn_down_t) tone(SPEAKER_PIN,1000+btn_down_t*20);
      else noTone(SPEAKER_PIN);

      //ring
      unsigned int target_i = ring_state/STRIP_NUM_VIRTUAL_PER_LED;
      unsigned long shade   = ring_state%STRIP_NUM_VIRTUAL_PER_LED;
      unsigned int off_i = (target_i+(STRIP_NUM_LEDS-1))%STRIP_NUM_LEDS;

      unsigned long t_r;
      unsigned long t_g;
      unsigned long t_b;
      unsigned long r;
      unsigned long g;
      unsigned long b;
      if(btn_down)
      {
        t_r = PLAYER_R;
        t_g = PLAYER_G;
        t_b = PLAYER_B;
        ring_state = (ring_state+STRIP_NUM_VIRTUAL_LEDS-MODE_SIGNUP_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }
      else
      {
        t_r = OPPO_R;
        t_g = OPPO_G;
        t_b = OPPO_B;
        ring_state = (ring_state+MODE_SIGNUP_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }

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
      FastLED.show();
      FastLED.delay(1);
      strip_leds[target_i] = 0x000000;
      strip_leds[off_i]    = 0x000000;
    }
    break;
    case MODE_PLAY:
    {
      noTone(SPEAKER_PIN);

      //ring
      unsigned int target_i = ring_state/STRIP_NUM_VIRTUAL_PER_LED;
      unsigned long shade   = ring_state%STRIP_NUM_VIRTUAL_PER_LED;
      unsigned int off_i = (target_i+(STRIP_NUM_LEDS-1))%STRIP_NUM_LEDS;

      unsigned long t_r;
      unsigned long t_g;
      unsigned long t_b;
      unsigned long r;
      unsigned long g;
      unsigned long b;
      if(btn_down)
      {
        t_r = PLAYER_R;
        t_g = PLAYER_G;
        t_b = PLAYER_B;
        ring_state = (ring_state+STRIP_NUM_VIRTUAL_LEDS-MODE_PLAY_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }
      else
      {
        t_r = OPPO_R;
        t_g = OPPO_G;
        t_b = OPPO_B;
        ring_state = (ring_state+MODE_PLAY_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }

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
      FastLED.show();
      FastLED.delay(1);
      strip_leds[target_i] = 0x000000;
      strip_leds[off_i]    = 0x000000;
    }
    break;
    case MODE_SCORE:
    {
      noTone(SPEAKER_PIN);

      //ring
      unsigned int target_i = ring_state/STRIP_NUM_VIRTUAL_PER_LED;
      unsigned long shade   = ring_state%STRIP_NUM_VIRTUAL_PER_LED;
      unsigned int off_i = (target_i+(STRIP_NUM_LEDS-1))%STRIP_NUM_LEDS;

      unsigned long t_r;
      unsigned long t_g;
      unsigned long t_b;
      unsigned long r;
      unsigned long g;
      unsigned long b;
      if(btn_down)
      {
        t_r = PLAYER_R;
        t_g = PLAYER_G;
        t_b = PLAYER_B;
        ring_state = (ring_state+STRIP_NUM_VIRTUAL_LEDS-MODE_SCORE_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }
      else
      {
        t_r = OPPO_R;
        t_g = OPPO_G;
        t_b = OPPO_B;
        ring_state = (ring_state+MODE_SCORE_SPEED)%STRIP_NUM_VIRTUAL_LEDS;
      }

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
      FastLED.show();
      FastLED.delay(1);
      strip_leds[target_i] = 0x000000;
      strip_leds[off_i]    = 0x000000;
    }
    break;
  }

  cmd_loop();
}

