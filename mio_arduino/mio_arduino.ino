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
#define CMD_DATA '1'

//btns
char btn_a_delta;
unsigned char btn_a_down;
char btn_b_delta;
unsigned char btn_b_down;

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

  unsigned int dcmd_n = 0;
  if(!serial_spinread(&d)) return;
  dcmd_n = d;

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
    //just don't do anything bc this func is currently bogus
    dcmd_n--;
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

  //init btns
  pinMode(BTN_A_PIN,INPUT);
  digitalWrite(BTN_A_PIN,LOW);
  pinMode(BTN_B_PIN,INPUT);
  digitalWrite(BTN_B_PIN,LOW);
  btn_a_delta = 0;
  btn_a_down = 0;
  btn_b_delta = 0;
  btn_b_down = 0;

  cmd_trigger_i = 0;
}

void loop()
{
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

  cmd_loop();
}

