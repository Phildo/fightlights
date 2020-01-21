#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 64 //0-128

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

CRGB dampen_color(CRGB in, int amt, int maxamt)
{
  int bright = 2; //takes away the lowest fractions (turns 1/10 into 2/11)
  int r = in.r*(amt+bright)/(maxamt+bright);
  int g = in.g*(amt+bright)/(maxamt+bright);
  int b = in.b*(amt+bright)/(maxamt+bright);
  return CRGB(r,g,b);
}

void initlut()
{
  //helpful constants for color-picking
  CRGB black      = CRGB(0x00,0x00,0x00); //CRGB::Black;
  CRGB white      = CRGB(0xFF,0xFF,0xFF); //CRGB::White;
  CRGB red        = CRGB(0xFF,0x00,0x00); //CRGB::Red;
  CRGB green      = CRGB(0x00,0xFF,0x00); //CRGB::Green;
  CRGB blue       = CRGB(0x00,0x00,0xFF); //CRGB::Blue;
  CRGB yellow     = CRGB(0xFF,0xFF,0x00); //CRGB::Yellow;
  CRGB light_blue = CRGB(0x4E,0xFD,0xEE);
  CRGB dark_blue  = CRGB(0x3C,0x44,0xE8);
  CRGB pink       = CRGB(0xF6,0x49,0xC7);

  lut[0x0] = white;                      //color_ball{,_fade[0]}
  lut[0x1] = dampen_color(lut[0x0],2,3); //color_ball_fade[1]
  lut[0x2] = dampen_color(lut[0x0],1,3); //color_ball_fade[2]
  lut[0x3] = black;                      //color_{clear,ball_fade[3]}
  lut[0x4] = light_blue;                 //color_a{,_fade[0]}
  lut[0x5] = dampen_color(lut[0x4],2,3); //color_a_fade[1]
  lut[0x6] = dampen_color(lut[0x4],1,3); //color_a_fade[2]
  lut[0x7] = dampen_color(lut[0x4],0,3); //color_a_fade[3]
  lut[0x8] = light_blue;                 //color_b{,_fade[0]}
  lut[0x9] = dampen_color(lut[0x8],2,3); //color_b_fade[1]
  lut[0xA] = dampen_color(lut[0x8],1,3); //color_b_fade[2]
  lut[0xB] = dampen_color(lut[0x8],0,3); //color_b_fade[3]
  lut[0xC] = (((pink/2)/2)/2);           //color_zone
  lut[0xD] = red;                        //color_?
  lut[0xE] = green;                      //color_?
  lut[0xF] = blue;                       //color_?
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

