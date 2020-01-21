#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 128 //0-128

//arduino constants
#define STRIP_LED_PIN 2

//strip constants
#define STRIP_NUM_LEDS 300
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER BRG

//sync w/ pi
#define FLUSH_TRIGGER "FLUSH"
#define BAUD_RATE 1000000

//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB lut[16];

//serial
int buff_n;
byte *buff;
int buff_i;
int trigger_i;

void initlut()
{
  lut[0x0] = CRGB(0,0,0);
  lut[0x1] = CRGB(0,20,0);
  lut[0x2] = CRGB(0,40,0);
  lut[0x3] = CRGB(0,60,0);
  lut[0x4] = CRGB(20,80,0);
  lut[0x5] = CRGB(40,100,0);
  lut[0x6] = CRGB(60,120,0);
  lut[0x7] = CRGB(80,140,20);
  lut[0x8] = CRGB(100,160,40);
  lut[0x9] = CRGB(120,180,60);
  lut[0xA] = CRGB(140,200,80);
  lut[0xB] = CRGB(160,220,100);
  lut[0xC] = CRGB(180,240,120);
  lut[0xD] = CRGB(200,0,140);
  lut[0xE] = CRGB(220,0,160);
  lut[0xF] = CRGB(240,0,180);
}

void buffToStrip()
{
  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
    strip_leds[i*2  ] = lut[buff[i]&0x0F];
    strip_leds[i*2+1] = lut[buff[i]>>4];
  }
}

void writestuff()
{
  //String msg = "this is a test";
  //buff_n = msg.length();
  //msg.getBytes(out_bytes,buff_n+1);
  //out_bytes[buff_n] = '\0'; buff_n++;
  //Serial.write(out_bytes,buff_n);
}

void setup()
{
  delay(300); //power-up safety delay
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  initlut();

  buff_n = STRIP_NUM_LEDS/2+strlen(FLUSH_TRIGGER);
  buff = (byte *)malloc(sizeof(byte)*buff_n+1);
  memset(buff,0,sizeof(byte)*buff_n+1);
  strcpy((char *)buff+(buff_n-strlen(FLUSH_TRIGGER)),FLUSH_TRIGGER); //actually unnecessary

  buff_i = 0;
  trigger_i = 0;

  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = lut[0];

  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  buffToStrip();
  FastLED.show();
}

void loop()
{
  while(Serial.available() > 0)
  {
    buff[buff_i++] = Serial.read();
    if(trigger_i && buff[buff_i-1] == FLUSH_TRIGGER[trigger_i])
    {
      trigger_i++;
      if(trigger_i == strlen(FLUSH_TRIGGER))
      {
        if(buff_i == buff_n)
        {
          buffToStrip();
          FastLED.show();
        }
        buff_i = 0;
        trigger_i = 0;
      }
    }
    else if(buff[buff_i-1] == FLUSH_TRIGGER[0]) trigger_i = 1;
    else                                        trigger_i = 0;

    if(buff_i == buff_n) buff_i = 0;
  }
}

