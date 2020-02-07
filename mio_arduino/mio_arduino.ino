#include <SoftwareSerial.h>
#include <NeoSWSerial.h>

//arduino constants
  //btns
#define BTN_A_PIN 8
#define BTN_A_RX_PIN 6
#define BTN_A_TX_PIN 2
#define BTN_B_PIN 9
#define BTN_B_RX_PIN 7
#define BTN_B_TX_PIN 3

//serial constants (sync w/ pi)
#define BAUD_RATE 1000000
#define AID "MIO\n"
#define CMD_PREAMBLE "CMD_"
#define T_CONSIDERED_DEAD 10000

//softserial constants (sync w/btns)
#define SOFT_BAUD_RATE 9600

//enum
#define CMD_WHORU '0'
#define CMD_DATA '1'

#define DEBUG_LED_PIN A2

//softser
NeoSWSerial btn_a_ser(BTN_A_RX_PIN,BTN_A_TX_PIN);
NeoSWSerial btn_b_ser(BTN_B_RX_PIN,BTN_B_TX_PIN);

//btns
char btn_a_delta;
unsigned char btn_a_down;
char btn_b_delta;
unsigned char btn_b_down;

//game state
char state;

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
  char d;
  if(!serial_spinread(&d)) return;

  state = d;
  for(int i = 0; i < 10; i++) //idempotent state, so just make sure it gets sent
  {
    btn_a_ser.write(state);
    btn_b_ser.write(state);
  }
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

  /*
  //init softserial //ALREADY DONE- MUST BE DONE AT DECLARE TIME
  btn_a_ser = NeoSWSerial(BTN_A_RX_PIN,BTN_A_TX_PIN);
  btn_b_ser = NeoSWSerial(BTN_B_RX_PIN,BTN_B_TX_PIN);
  */

  btn_a_ser.begin(SOFT_BAUD_RATE);
  btn_b_ser.begin(SOFT_BAUD_RATE);

  //init btns
  pinMode(BTN_A_PIN,INPUT);
  digitalWrite(BTN_A_PIN,LOW);
  pinMode(BTN_B_PIN,INPUT);
  digitalWrite(BTN_B_PIN,LOW);
  btn_a_delta = 0;
  btn_a_down = 0;
  btn_b_delta = 0;
  btn_b_down = 0;

  pinMode(DEBUG_LED_PIN,OUTPUT);
  digitalWrite(DEBUG_LED_PIN,LOW);

  cmd_trigger_i = 0;
}

void loop()
{
  btn_a_delta = 0;
  //if(digitalRead(BTN_A_PIN)) { if(!btn_a_down) btn_a_delta =  1; btn_a_down = 1; }
  //else                       { if( btn_a_down) btn_a_delta = -1; btn_a_down = 0; }
  if(btn_a_ser.available()) { btn_a_down = btn_a_ser.read(); btn_a_delta = btn_a_down*2-1; };

  if(btn_a_delta)
  {
    //if(btn_a_down) digitalWrite(DEBUG_LED_PIN,HIGH);
    //else           digitalWrite(DEBUG_LED_PIN,LOW);
    digitalWrite(DEBUG_LED_PIN,HIGH);
  }

  btn_b_delta = 0;
  //if(digitalRead(BTN_B_PIN)) { if(!btn_b_down) btn_b_delta =  1; btn_b_down = 1; }
  //else                       { if( btn_b_down) btn_b_delta = -1; btn_b_down = 0; }
  if(btn_b_ser.available()) { btn_b_down = btn_b_ser.read(); btn_b_delta = btn_b_down*2-1; };

  //relay btns to pi
  byte msg = 0x00;
  msg |= (btn_a_delta & 0x0F);
  msg |= (btn_b_delta << 4);
  if(msg) Serial.write(&msg,1);

  cmd_loop();
}

