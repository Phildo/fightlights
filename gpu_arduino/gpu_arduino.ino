




gpu buff i: 126
BEGIN
067 43 C
077 4d M
068 44 D
095 5f _
049 31 1
009 09
000 00
005 05
102 66 f
000 00
000 00
062 3e >
006 06
014 0e
050 32 2
004 04
011 0b

039 27 '
004 04
009 09
034 22 "
003 03
008 08
001 01
004 04
033 21 !
003 03
007 07
000 00
008 08
034 22 "
003 03
008 08
040 28 (
004 04
009 09
051 33 3
005 05
012 0c

062 3e >
006 06
014 0e
066 42 B
006 06
015 0f
057 39 9
005 05
013 0d
045 2d -
004 04
010 0a

036 24 $
003 03
008 08
001 01
003 03
033 21 !
003 03
007 07
001 01
026 1a
102 66 f
000 00
000 00
001 01
234 ea ▒
000 00
000 00
000 00
000 00
011 0b

102 66 f
000 00
000 00
033 21 !
003 03
007 07
033 21 !
003 03
007 07
036 24 $
003 03
008 08
045 2d -
004 04
010 0a

057 39 9
005 05
013 0d
066 42 B
006 06
015 0f
062 3e >
006 06
014 0e
051 33 3
005 05
012 0c

000 00
000 00
102 66 f
034 22 "
003 03
008 08
001 01
004 04
033 21 !
003 03
007 07
000 00
005 05
028 1c
008 08
008 08
032 20
017 11
016 10
041 29 )
017 11
017 11
052 34 4
016 10
017 11
055 37 7
016 10
018 12
END









#include <FastLED.h>

//customize
#define STRIP_BRIGHTNESS 255 //0-255

//arduino constants
#define STRIP_LED_PIN 3

//strip constants
#define STRIP_NUM_LEDS 300
#define STRIP_LED_TYPE WS2812
#define STRIP_COLOR_ORDER GRB

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

