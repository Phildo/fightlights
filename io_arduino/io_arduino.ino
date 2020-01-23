//#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 128 //0-128

//arduino constants
  //strip
#define STRIP_LED_PIN 3
  //btns
#define BTN_A_PIN 8
#define BTN_B_PIN 9
  //mic
#define MIC_PIN 2
#define MIC_OUT_PIN 10

//strip constants
#define STRIP_NUM_LEDS 16
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER RGB

//sync w/ pi
#define BAUD_RATE 1000000

//strip
//CRGB strip_leds[STRIP_NUM_LEDS];
//CRGB clear;

//btns
char btn_a_delta;
unsigned char btn_a_down;
char btn_b_delta;
unsigned char btn_b_down;

//mic
unsigned int mic_hot;
volatile unsigned int im_mic_hot;
void mic_interrupt() { im_mic_hot = 1000; }

void setup()
{
  delay(300); //power-up safety delay

  //init serial
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  //init strip
  //clear = CRGB(0x00,0x00,0x00);
  //for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;
  //FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  //FastLED.setBrightness(STRIP_BRIGHTNESS);
  //FastLED.show();

  //init btns
  pinMode(BTN_A_PIN,INPUT_PULLUP);
  pinMode(BTN_B_PIN,INPUT_PULLUP);
  btn_a_delta = 0;
  btn_a_down = 0;
  btn_b_delta = 0;
  btn_b_down = 0;

  //mic
  pinMode(MIC_PIN,INPUT);
  //digitalWrite(MIC_PIN,0);
  /*
    apparently RPi (compilation src) ships with very old version of arduino, and
    "digitalPinToInterrupt" is erroneously missing in that version. I don't
    want to mess with this compilation pipeline I don't understand, so am doing
    the "unrecommended" thing of just using the pin directly ("should" be safe for
    newer arduinos...)
  */
  //attachInterrupt(digitalPinToInterrupt(MIC_PIN), mic_interrupt, RISING);
    //attachInterrupt(                      MIC_PIN , mic_interrupt, RISING);
    attachInterrupt(                      0, mic_interrupt, RISING);
  mic_hot = 0;
  im_mic_hot = 0;
  pinMode(MIC_OUT_PIN,OUTPUT);
  digitalWrite(MIC_OUT_PIN,0);
}

void loop()
{
  //mic_hot = (digitalRead(MIC_PIN) || im_mic_hot);
  mic_hot = im_mic_hot;
  if(im_mic_hot) im_mic_hot--;

  btn_a_delta = 0;
  if(mic_hot || !digitalRead(BTN_A_PIN)) { if(!btn_a_down) btn_a_delta =  1; btn_a_down = 1; }
  else                                   { if(btn_a_down)  btn_a_delta = -1; btn_a_down = 0; }

  btn_b_delta = 0;
  if(mic_hot || !digitalRead(BTN_B_PIN)) { if(!btn_b_down) btn_b_delta =  1; btn_b_down = 1; }
  else                                   { if(btn_b_down)  btn_b_delta = -1; btn_b_down = 0; }

  digitalWrite(MIC_OUT_PIN,mic_hot);

  byte msg = 0x00;
  msg |= (btn_a_delta & 0x0F);
  msg |= (btn_b_delta << 4);
  if(msg) Serial.write(&msg,1);
}

