// BOARD: ARDUINO MEGA 2560

/*
Pins:
????
*/


///////////////////////////////////////////////////
// READER 4 DIABLED IN 2 PLACES BELOW
//////////////////////////////////////////////////

#define MASTER_ADDR 'M'
#define GET_CURRENT_READING 'R'
#define GET_LAST_READING 'L'
#define DUMP_READING 'D'
#define STOP_DUMP 'S'
#define RESET_BALL_PRESENCE_THRESH 'T' // wait 1500ms after receiving OK
#define RESET_BUCKET_COLOR 'B'
#define KIDS_GONE_CHECK 'K'

// valid responses:
// (update is_valid() when adding
#define RED 'r'
#define GREEN 'g'
#define BLUE 'b'
#define YELLOW 'y'
#define ORANGE 'o'
#define NONE 'n'
#define OK 'K'
#define YES 'Y'
#define NO 'N'
#define ERROR 'E'

#define ledPin A5 // ????????????

#define FULL_TURN 1600
#define A_POS 700
#define B_POS 850
#define C_POS 975
#define D_POS 1175

// last index (in targets[]) that each color was thrown to..
int last_red_index= 0;
int last_yellow_index= 1;
int last_blue_index= 2;
int last_orange_index= 2;
int last_green_index= 2;


//keep track of how long it's been since a ball was thrown
unsigned long last_throw_time= 0;
unsigned long last_tamper_time= 0;
unsigned long last_kids_gone_check= millis();

char targets[3] = {RED, YELLOW, BLUE};
int target_pos[3]= {A_POS, B_POS, C_POS};

void setup(){
  Serial.begin(9600);
  Serial.println(); // break from old dumps-
  Serial.println("fake controller starting up...");
  setup_rs485_tranceiver();
    
  pinMode(ledPin, OUTPUT);
      
  Serial.println("blinking LED...");
  blinkLed();
  
}

void loop(){
  // get a reading
  for(int i=3; i<=4; i++){
    send_command(i, GET_CURRENT_READING);
    char reading= get_response();
  }
  //Serial.flush();
}

void send_command(int addr, char cmd){
  char msg[3];
  msg[0]= addr+48; //convert to ascii caracter
  msg[1]= cmd;
  msg[2]= '\0';
  
  while(Serial.available()>0) Serial.read(); // clear incoming buffer, expecting new stuff
  tx_msg(msg);
}

char get_response(){
  char response= ERROR;
  unsigned long start_time= millis();
  
  // wait for response to arrive or timeout
  while(Serial.available() < 2 && millis() < start_time+200);
  
  // if timeout
  if (millis() >= start_time+200){
    Serial.println(": timeout");
    return response;
  }
  
  // iterate through incoming buffer, looking for an 'M'
  while(Serial.read() != 'M'){
    if (Serial.available()==0){
      Serial.println(": no M");
      return response;
    }
  }
  
  // next character after the 'M' should be our response
  response= Serial.read();
  //if(!is_valid(response)) response= ERROR;
  
  Serial.print(": ");
  Serial.println(response);
  if (response == -1) reset_bus();
  return response;
}

void reset_bus(){
  Serial.println("flushing in/out buffers...");
  Serial.flush();
  while(Serial.available()>0) Serial.read();
}

boolean is_ball(char candidate){
  if(candidate == ORANGE) return true;
  if(candidate == GREEN) return true;
  if(candidate == YELLOW) return true;
  if(candidate == RED) return true;
  if(candidate == BLUE) return true;
  return false;
}

boolean is_valid(char candidate){
  if(candidate == ORANGE) return true;
  if(candidate == GREEN) return true;
  if(candidate == YELLOW) return true;
  if(candidate == OK) return true;
  if(candidate == YES) return true;
  if(candidate == NO) return true;
  if(candidate == NONE) return true;
  if(candidate == ERROR) return true; 
  return false;
}  

void blinkLed(){
  for(int i=0; i<3; i++){
    digitalWrite(ledPin, HIGH);
    delay(250);
    digitalWrite(ledPin, LOW);
    delay(250);
  }
}


