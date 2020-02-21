#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 255 //0-255

//arduino constants
#define STRIP_LED_PIN 3

//strip constants
#define STRIP_NUM_LEDS 300
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER RGB

//serial constants (sync w/ pi)
#define BAUD_RATE 1000000
#define AID "GPU\n"
#define CMD_PREAMBLE "CMD_"
#define T_CONSIDERED_DEAD 10000

//enum
#define CMD_WHORU '0'
#define CMD_DATA '1'

#define ENC_STREAM 0
#define ENC_RUN 1

//strip
CRGB strip_leds[STRIP_NUM_LEDS];
CRGB clear;

//cmdloop
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
  if(!serial_spinread(&d)) return;
  dcmd_n = d;

  int strip_i = 0;
  while(dcmd_n) //perform commands
  {
    if(!serial_spinread(&d)) return;
    if(d == ENC_STREAM)
    {
      if(!serial_spinread(&d)) return;
      char n = d;
      while(n)
      {
        if(!serial_spinread(&d)) return;
        char r = d;
        if(!serial_spinread(&d)) return;
        char g = d;
        if(!serial_spinread(&d)) return;
        char b = d;
        CRGB color = CRGB(r,g,b);
        strip_leds[strip_i++] = color;
        n--;
      }
    }
    else if(d == ENC_RUN)
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
    }
    dcmd_n--;
  }

  FastLED.show();
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
          case CMD_DATA:  cmd_data(); break;
        }
      }
    }
  }
}

void setup()
{
  delay(300); //power-up safety delay

  //kill debug LED
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  clear = CRGB(0,0,0);
  for(int i = 0; i < STRIP_NUM_LEDS; i++) strip_leds[i] = clear;

  FastLED.addLeds<STRIP_LED_TYPE, STRIP_LED_PIN, STRIP_COLOR_ORDER>(strip_leds, STRIP_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(STRIP_BRIGHTNESS);
  FastLED.setDither(0);
  FastLED.show();
}

void loop()
{
  cmd_loop();
}

