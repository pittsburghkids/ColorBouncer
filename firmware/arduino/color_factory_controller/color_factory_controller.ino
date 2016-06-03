// BOARD: ARDUINO MEGA 2560

/*
Pins:
????
*/


#define MASTER_ADDR 'M'
#define GET_CURRENT_READING 'R'
#define DUMP_READING 'D'
#define STOP_DUMP 'S'
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
#define C_POS 1000
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

char targets[3] = {RED, GREEN, BLUE};
char defaultColors[3] = {RED, GREEN, BLUE};
int target_pos[3]= {A_POS, B_POS, C_POS};

void setup(){
  Serial.begin(9600);
  Serial.println(); // break from old dumps-
  Serial.println("starting up...");
  setup_rs485_tranceiver();
    
  pinMode(ledPin, OUTPUT);
  setup_stepper(B_POS);
    
  //Serial.println("resetting bucket colors...");
  //reset_bucket_colors();
  
  Serial.println("blinking LED...");
  blinkLed();
  
  test_bucket_lights();
}

void loop(){
  //dump_motor_data();
  
  // read reader sensors and update target lights
  update_targets();
  
  // get a reading
  send_command(1, GET_CURRENT_READING);
  char reading= get_response();

  // if reading shows ball, aim bouncer.
  // bouncer is already aimed at target, move it a bit to let us know it cares
  if(is_ball(reading) && millis() > last_throw_time + 500){
    if(motor_is_at(get_target(reading))) goto_pos(get_target(reading)+FULL_TURN/16);
    goto_pos(get_target(reading));
    last_throw_time= millis();
  }
  tamper_check();
  
  // if things are quiet, re-home stepper
  if(millis() > last_throw_time + 300000){ //5 minutes
    calibrateStepper(B_POS);
    last_throw_time= millis();
  }
}

// update targets[] and the leds above the target hoppers
void update_targets(){
  // get colors from sensors
  for(int i=2; i<=4; i++){
    send_command(i, GET_CURRENT_READING);
    char color= get_response();
    // store in targets[]
    if (is_ball(color)){
      targets[i-2]= color;
      flashTarget(i-2, true);
    }
    if (color == NONE){
      targets[i-2]= defaultColors[i-2];
      flashTarget(i-2, false);
    }
  }
  
  // set bucket leds to match
  update_bucket_lights();
}

short kids_gone(){
  int tries=0;
  if(millis() > last_kids_gone_check+10000){
    last_kids_gone_check= millis();
    char response= 'E';
    for(int i=2; i<=4; i++){ // don't ask the dropper
      do{
        send_command(i, KIDS_GONE_CHECK);
        if (tries>3) delay(100);
        tries++;
        response= get_response();
      } while (response=='E');
      if(response=='N') return false;
    }
    return true;
  }
  else return false;
}


void reset_bucket_colors(){
  for(int i=2; i<=4; i++){
    do send_command(i, RESET_BUCKET_COLOR); 
    while (get_response()=='E');
  }
}  

int get_target(char color){
  if(color == ORANGE){
    for(int i=0; i<3; i++){
      last_orange_index++;
      if (last_orange_index > 2) last_orange_index=0;
      if (targets[last_orange_index] == ORANGE) return target_pos[last_orange_index];
    }
    return D_POS;
  }
  
  if(color == YELLOW){
    for(int i=0; i<3; i++){
      last_yellow_index++;
      if (last_yellow_index > 2) last_yellow_index=0;
      if (targets[last_yellow_index] == YELLOW) return target_pos[last_yellow_index];
    }
    return D_POS;
  }
  
  if(color == GREEN){
    for(int i=0; i<3; i++){
      last_green_index++;
      if (last_green_index > 2) last_green_index= 0;
      if (targets[last_green_index] == GREEN) return target_pos[last_green_index];
    }
    return D_POS;
  }
  
  if(color == RED){
    for(int i=0; i<3; i++){
      last_red_index++;
      if (last_red_index > 2) last_red_index=0;
      if (targets[last_red_index] == RED) return target_pos[last_red_index];
    }
    return D_POS;
  }
  
  if(color == BLUE){
    for(int i=0; i<3; i++){
      last_blue_index++;
      if (last_blue_index > 2) last_blue_index=0;
      if (targets[last_blue_index] == BLUE) return target_pos[last_blue_index];
    }
    return D_POS;
  }
}

void send_command(int addr, char cmd){
  char msg[3];
  msg[0]= addr+48; //convert to ascii caracter
  msg[1]= cmd;
  msg[2]= '\0';
  
  while(Serial.available()>0) Serial.read(); // clear incoming buffer, expecting new stuff
  tx_msg(msg);}

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
  //if(candidate == ORANGE) return true;
  if(candidate == GREEN) return true;
  //if(candidate == YELLOW) return true;
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

void dump_sensors(){
  for(int i=1; i<5; i++) dump_sensor(i);
  Serial.println();
}

void dump_sensor(int addr){
  send_command(addr, DUMP_READING);
  char incoming=0;
  unsigned long start_time=millis();
  
  Serial.print(" Sensor #");
  Serial.print(addr);
  Serial.print(": ");
  
  while(incoming != '\n'){
    if(Serial.available()>0){
      incoming= (Serial.read());
      Serial.write(incoming);
    }
    if(millis()>start_time+200) break;
  }
}



