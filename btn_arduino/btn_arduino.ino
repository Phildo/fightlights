#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 255 //0-255
#define BTN_DOWN_T_MAX 300

//strip constants
#define STRIP_NUM_LEDS 16
#define STRIP_NUM_VIRTUAL_PER_LED 10
#define STRIP_NUM_VIRTUAL_LEDS (STRIP_NUM_LEDS*STRIP_NUM_VIRTUAL_PER_LED)
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER GRB

//pins
// out
#define STRIP_LED_PIN 3
#define LED_PIN 4
#define SPEAKER_PIN 5
// in
#define BTN_PIN 8
#define DATA_TRANSITION_PIN 9
#define DATA_PIN_0 10
#define DATA_PIN_1 11
#define DATA_PIN_2 12

//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB clear;

//state
unsigned int ring_state;
char btn_delta;
unsigned char btn_down;
int btn_down_t;
unsigned char data_transitioning;
unsigned char data;

void setup()
{
  delay(300); //power-up safety delay

  //out
  //pinMode(STRIP_LED_PIN,OUTPUT); //defer to FastLED
  pinMode(LED_PIN,OUTPUT);
  pinMode(SPEAKER_PIN,OUTPUT);
  //in
  pinMode(BTN_PIN,INPUT_PULLUP);
  pinMode(DATA_TRANSITION_PIN,INPUT_PULLUP);
  pinMode(DATA_PIN_0,INPUT_PULLUP);
  pinMode(DATA_PIN_1,INPUT_PULLUP);
  pinMode(DATA_PIN_2,INPUT_PULLUP);

  //init strip
  clear = CRGB(0x00,0x00,0x00);
  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;
  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  FastLED.show();

  //init state
  ring_state = 0;
  btn_delta = 0;
  btn_down = 0;
  btn_down_t = 0;
  data_transitioning = 0;
  data = 0;
}

void loop()
{
  btn_delta = 0;
  if(!digitalRead(BTN_PIN)) { if(!btn_down) { btn_delta =  1; btn_down_t = BTN_DOWN_T_MAX/10; } btn_down = 1; btn_down_t++;  if(btn_down_t > BTN_DOWN_T_MAX) btn_down_t = BTN_DOWN_T_MAX*9/10; }
  else                      { if( btn_down) { btn_delta = -1;                                 } btn_down = 0; btn_down_t-=3; if(btn_down_t < 0             ) btn_down_t = 0;                   }

  if(btn_delta)
  {
    //if(btn_down) digitalWrite(LED_PIN,LOW);
    //else         digitalWrite(LED_PIN,HIGH);
  }

  if(btn_down_t) tone(SPEAKER_PIN,1000+btn_down_t*20);
  else noTone(SPEAKER_PIN);

  unsigned int target_i = ring_state/STRIP_NUM_VIRTUAL_PER_LED;
  unsigned long shade   = ring_state%STRIP_NUM_VIRTUAL_PER_LED;
  unsigned int off_i = (target_i+(STRIP_NUM_LEDS-1))%STRIP_NUM_LEDS;
  shade = shade*0xFF/STRIP_NUM_VIRTUAL_PER_LED;

  unsigned long target_color;
  if(btn_down)
  {
    target_color = shade << 8;
    strip_leds[target_i] = target_color;
    strip_leds[off_i] = 0x00FF00 - target_color;
    ring_state = (ring_state+STRIP_NUM_VIRTUAL_LEDS-1)%STRIP_NUM_VIRTUAL_LEDS;
  }
  else
  {
    target_color = shade << 16;
    strip_leds[target_i] = target_color;
    strip_leds[off_i] = 0xFF0000 - target_color;
    ring_state = (ring_state+1)%STRIP_NUM_VIRTUAL_LEDS;
  }
  FastLED.show();
  FastLED.delay(1);
  strip_leds[target_i] = 0x000000;
  strip_leds[off_i]    = 0x000000;
}

