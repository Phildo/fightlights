//arduino constants
  //btns
#define BTN_A_PIN 8
#define BTN_B_PIN 9

//sync w/ pi
#define BAUD_RATE 1000000

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

}

void loop()
{
  btn_a_delta = 0;
  if(digitalRead(BTN_A_PIN)) { if(!btn_a_down) btn_a_delta =  1; btn_a_down = 1; }
  else                       { if(btn_a_down)  btn_a_delta = -1; btn_a_down = 0; }

  btn_b_delta = 0;
  if(digitalRead(BTN_B_PIN)) { if(!btn_b_down) btn_b_delta =  1; btn_b_down = 1; }
  else                       { if(btn_b_down)  btn_b_delta = -1; btn_b_down = 0; }

  byte msg = 0x00;
  msg |= (btn_a_delta & 0x0F);
  msg |= (btn_b_delta << 4);
  if(msg) Serial.write(&msg,1);
}

