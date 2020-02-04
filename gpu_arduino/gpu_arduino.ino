#include <FastLED.h>

//enum
#define CMD_WHORU '0'
#define CMD_DATA '1'

//customize
#define AID "GPU"
#define STRIP_BRIGHTNESS 100 //0-255
#define STRIP_MAX_BRIGHTNESS 256
#define STRIP_MIN_BRIGHTNESS 0

//arduino constants
#define STRIP_LED_PIN 3

//strip constants
#define STRIP_NUM_LEDS 300
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER RGB

//serial constants (sync w/ pi)
#define CMD_PREAMBLE "CMD_"
#define BAUD_RATE 1000000
#define T_CONSIDERED_DEAD 10000

//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB lut[16];

//data
size_t dbuff_s;
byte *dbuff;
unsigned int dbuff_i;

CRGB dampen_color(CRGB in, int amt, int maxamt)
{
  int bright = 1; //takes away the lowest fractions (turns 1/10 into 2/11)
  amt    += bright;
  maxamt += bright;
  int r = in.r*amt/maxamt;
  int g = in.g*amt/maxamt;
  int b = in.b*amt/maxamt;
  return CRGB(r,g,b);
}

CRGB hdampen_color(CRGB in, int amt, int maxamt)
{
  int bright = 1; //takes away the lowest fractions (turns 1/10 into 2/11)
  amt    += bright;
  maxamt += bright;
  amt = maxamt-amt;
  int r = in.r;
  int g = in.g;
  int b = in.b;
  for(int i = 0; i < amt; i++)
  {
    r/=3;
    g/=3;
    b/=3;
  }
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
  CRGB pink       = CRGB(0xA6,0x11,0x27);

  lut[0x0] = black;                      //color_clear
  lut[0x1] = white;                      //color_ball{,_fade[0]}
  lut[0x2] = hdampen_color(lut[0x1],2,3); //color_ball_fade[1]
  lut[0x3] = hdampen_color(lut[0x1],1,3); //color_ball_fade[2]
  lut[0x4] = hdampen_color(lut[0x1],0,3); //color_ball_fade[3]
  lut[0x5] = blue;                       //color_a{,_fade[0]}
  lut[0x6] = hdampen_color(lut[0x5],2,3); //color_a_fade[1]
  lut[0x7] = hdampen_color(lut[0x5],1,3); //color_a_fade[2]
  lut[0x8] = hdampen_color(lut[0x5],0,3); //color_a_fade[3]
  lut[0x9] = red;                        //color_b{,_fade[0]}
  lut[0xA] = hdampen_color(lut[0x9],2,3); //color_b_fade[1]
  lut[0xB] = hdampen_color(lut[0x9],1,3); //color_b_fade[2]
  lut[0xC] = hdampen_color(lut[0x9],0,3); //color_b_fade[3]
  lut[0xD] = pink;                       //color_zone
  lut[0xE] = red;                        //color_?
  lut[0xF] = green;                      //color_?

  for(int i = 0; i < 0x10; i++)
  {
                 lut[i].r = min(lut[i].r,STRIP_MAX_BRIGHTNESS);
    if(lut[i].r) lut[i].r = max(lut[i].r,STRIP_MIN_BRIGHTNESS);
                 lut[i].g = min(lut[i].g,STRIP_MAX_BRIGHTNESS);
    if(lut[i].g) lut[i].g = max(lut[i].g,STRIP_MIN_BRIGHTNESS);
                 lut[i].b = min(lut[i].b,STRIP_MAX_BRIGHTNESS);
    if(lut[i].b) lut[i].b = max(lut[i].b,STRIP_MIN_BRIGHTNESS);
  }
}

void dbuffToStrip()
{
  for(int i = 0; i < STRIP_NUM_LEDS/2; i++)
  {
    strip_leds[i*2  ] = lut[buff[i]&0x0F];
    strip_leds[i*2+1] = lut[buff[i]>>4];
  }
}

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

void cmd_whoru()
{
  Serial.write(AID);
}

void cmd_data()
{
  char d;

  unsigned int dcmd_n = 0;
  for(int i = 0; i < 3; i++)
  {
    if(!serial_spinread(&d)) return;
    dcmd_n *= 10;
    dcmd_n += atoi(d);
  }

  int strip_i = 0;
  while(dcmd_n) //perform commands
  {
    if(!serial_spinread(&d)) return;
    char n = d;
    if(!serial_spinread(&d)) return;
    char r = d;
    if(!serial_spinread(&d)) return;
    char g = d;
    if(!serial_spinread(&d)) return;
    char b = d;
    CRGB color = CRGB(r,g,b);
    while(n)
    {
      strip_leds[strip_i++] = color;
      n--;
    }
    dcmd_n--;
  }

  FastLED.show();
}

void setup()
{
  delay(300); //power-up safety delay
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  initlut();

  dbuff_s = STRIP_NUM_LEDS/2+strlen(FLUSH_TRIGGER);
  buff = (byte *)malloc(sizeof(byte)*dbuff_s+1);
  memset(buff,0,sizeof(byte)*dbuff_s+1);
  strcpy((char *)buff+(dbuff_s-strlen(FLUSH_TRIGGER)),FLUSH_TRIGGER); //actually unnecessary

  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = lut[0];

  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  buffToStrip();
  FastLED.show();
}

void loop()
{
  char d;
  //get preamble
  for(int i = 0; i < strlen(CMD_PREAMBLE))
  {
    if(!serial_spinread(&d)) return;
    if(d != CMD_PREAMBLE[cmd_pre_i]) return;
  }
  //run command
  if(!serial_spinread(&d)) return;
  switch(d)
  {
    case CMD_WHORU: cmd_whoru(); break;
    case CMD_DATA:  cmd_data(); break;
  }
}

