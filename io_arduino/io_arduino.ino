#include "crs.inoh"

//arduino constants
  //btns
#define BTN_A_PIN 8
#define BTN_B_PIN 9

//serial constants (sync w/ pi)
#define BAUD_RATE 1000000
#define AID "MIO\n"
#define CMD_PREAMBLE "CMD_"
#define T_CONSIDERED_DEAD 10000

//enum
#define CMD_WHORU '0'

//btns
char btn_a_delta;
unsigned char btn_a_down;
char btn_b_delta;
unsigned char btn_b_down;

void setup()
{
  delay(300); //power-up safety delay

  //init serial
  Serial.begin(BAUD_RATE);
  while(!Serial) { ; }

  //init btns
  pinMode(BTN_A_PIN,INPUT);
  digitalWrite(BTN_A_PIN,LOW);
  pinMode(BTN_B_PIN,INPUT);
  digitalWrite(BTN_B_PIN,LOW);
  btn_a_delta = 0;
  btn_a_down = 0;
  btn_b_delta = 0;
  btn_b_down = 0;

  crs_call(0,CRSLBL_TICK,0);
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

#define CRSLBL_TICK 0
#define CRSLBL_GET_PREAMBLE 1
#define CRSLBL_GET_CMD 2
#define CRSLBL_SERIAL_SPINREAD 3

void gotolbl(stackv lbl)
{
  switch(lbl)
  {
    case CRSLBL_TICK:            goto lbl_tick;
    case CRSLBL_GET_PREAMBLE:    goto lbl_get_preamble;
    case CRSLBL_GET_CMD:         goto lbl_get_cmd;
    case CRSLBL_SERIAL_SPINREAD: goto lbl_serial_spinread;
  }
}

unsigned char crs_serial_spinread(char *c) //DON'T DIRECTLY CALL! ONLY CALL THROUGH CRS (func signature only used for reference)
{
  lbl_serial_spinread:
  CRPUSH(cmd_dead_t,0,-1);
  while(CRREAD(cmd_dead_t,-1) < T_CONSIDERED_DEAD)
  {
    if(Serial.available()) { *((char *)CRREAD(arg,-2)) = Serial.read(); CRPOP(cmd_dead_t,-1); crs_return(1); }
    goto lbl_yield;
    lbl_unyield:
    CRINC(cmd_dead_t,-1);
  }
  CRPOP(cmd_dead_t,-1);
  crs_return(0);
  return 0; //never gets here, just for compiler
}

void crs_tick()
{
  while(1)
  {
    lbl_tick:
    CRPUSH(d,-2,0);
    CRPUSH(i,-1)
    //get preamble
    for(; CRREAD(i,-1) < strlen(CMD_PREAMBLE); CRINC(i,-1))
    {
      if(crs_call(CRSLBL_GET_PREAMBLE,CRSLBL_SERIAL_SPINREAD,CRDEREF(d,-2)));
      lbl_get_preamble:
      if(!crs_returned()) { CRPOP(d,-2); CRPOP(i,-1); crs_return(0); }

      if(CRREAD(d,-2) != CMD_PREAMBLE[CRREAD(i,-1)]) { CRPOP(d,-2); CRPOP(i,-1); crs_return(0); }
    }
    //run command
    if(crs_call(CRSLBL_GET_CMD,CRSLBL_SERIAL_SPINREAD,CRDEREF(d,-2)));
    lbl_get_cmd:
    if(!crs_returned()) { CRPOP(d,-2); CRPOP(i,-1); crs_return(0); }

    switch(CRREAD(d,-2))
    {
      case CMD_WHORU: cmd_whoru(); break;
      case CMD_DATA:  cmd_data(); break;
    }
  }
}

void loop()
{
  goto lbl_unyield;
  lbl_yield:

  btn_a_delta = 0;
  if(digitalRead(BTN_A_PIN)) { if(!btn_a_down) btn_a_delta =  1; btn_a_down = 1; }
  else                       { if( btn_a_down) btn_a_delta = -1; btn_a_down = 0; }

  btn_b_delta = 0;
  if(digitalRead(BTN_B_PIN)) { if(!btn_b_down) btn_b_delta =  1; btn_b_down = 1; }
  else                       { if( btn_b_down) btn_b_delta = -1; btn_b_down = 0; }

  byte msg = 0x00;
  msg |= (btn_a_delta & 0x0F);
  msg |= (btn_b_delta << 4);
  if(msg) Serial.write(&msg,1);
}

