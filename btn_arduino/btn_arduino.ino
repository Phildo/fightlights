#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 255 //0-255
#define STRIP_UPDATE_WAIT 1

//strip constants
#define STRIP_NUM_LEDS 16
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER RGB

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
unsigned int ring_update_wait;
unsigned char ring_state;
char btn_delta;
unsigned char btn_down;
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
  ring_update_wait = STRIP_UPDATE_WAIT;
  ring_state = 0;
  btn_delta = 0;
  btn_down = 0;
  data_transitioning = 0;
  data = 0;
}

void loop()
{
  btn_delta = 0;
  if(!digitalRead(BTN_PIN)) { if(!btn_down) btn_delta =  1; btn_down = 1; }
  else                      { if(btn_down)  btn_delta = -1; btn_down = 0; }

  if(btn_delta)
  {
    //if(btn_down) digitalWrite(LED_PIN,LOW);
    //else         digitalWrite(LED_PIN,HIGH);
  }

  ring_update_wait--;
  if(ring_update_wait == 0)
  {
    ring_update_wait = STRIP_UPDATE_WAIT;
    //for(int i = 0; i < STRIP_NUM_LEDS; i++)
      //strip_leds[i] = random(0xFFFFFF);
    for(int i = 0; i < STRIP_NUM_LEDS; i++)
      strip_leds[i] = 0x000000;
    if(btn_down)
    {
      strip_leds[ring_state] = 0x00FF00;
      ring_state = (ring_state+STRIP_NUM_LEDS-1)%STRIP_NUM_LEDS;
    }
    else
    {
      strip_leds[ring_state] = 0xFF0000;
      ring_state = (ring_state+1)%STRIP_NUM_LEDS;
    }
    FastLED.show();
  }
  FastLED.delay(1);
}

