// interal interrupt interfacing
// for simultaneous solenoid release and bouncer move
#include <MsTimer2.h>

#define MASTER_ADDR 'M'
#define GET_CURRENT_READING 'R'
#define GET_LAST_READ 'L'
#define DUMP_READING 'D'
#define STOP_DUMP 'S'
#define RESET_BALL_PRESENCE_THRESH 'T' // wait 1500ms after receiving OK

// valid responses:
// (update is_valid() when adding
#define RED 'r'
#define GREEN 'g'
#define BLUE 'b'
#define YELLOW 'y'
#define ORANGE 'o'
#define OK 'K'
#define NONE 'n'
#define ERROR 'E'


#define ledPin 13
#define release_pin 6

#define FULL_TURN 1600
#define RED_POS 55
#define YELLOW_POS 205
#define BLUE_POS 355

//keep track of how long it's been since a ball was thrown
#include "WProgram.h"
void release_solenoid();
void setup();
void loop();
void throw_ball(int target);
void reset_sensor_thresh(int addr);
void wait(long mills);
void dump_sensor(int addr);
int get_target(char color);
void send_command(int addr, char cmd);
char get_response();
boolean is_ball(char candidate);
boolean is_valid(char candidate);
void blinkLed();
void setup_rs485_tranceiver();
void tx_msg(char *msg);
void setup_stepper();
void rest_check();
void home_stepper();
short needs_correction();
void correct_pos();
short encoder_reading_consistent();
void goto_encoder_half();
void dump_motor_data();
void goto_pos(long target);
void goto_pos_at_speed(long target, int spd);
short step(short dir);
void stepper_power(short enable);
unsigned long last_throw_time=0;

// this function called by internal timer interrupt
void release_solenoid(){
  digitalWrite(release_pin, LOW);
}

void setup(){
  Serial.begin(9600);
  Serial.println(); // break from old dumps-
  Serial.println("starting up...");
  setup_rs485_tranceiver();
  pinMode(ledPin, OUTPUT);
  pinMode(release_pin, OUTPUT);
  setup_stepper();
  if (needs_correction()) correct_pos();
  
  /*Serial.println("clearing balls...");
  set_release(HIGH);
  delay(5000);
  set_release(LOW);
  */
  Serial.println("reseting sensor thresholds...");
  reset_sensor_thresh(1);
  
  Serial.println("blinking LED...");
  blinkLed();
}

void loop(){
  // still to write: 
  // track bouncer grabs, and wait for kids to leave alone before throwing balls
    
  //dump_sensor(1);
  //dump_motor_data();
  
  
  
  // get a reading
  send_command(1, GET_CURRENT_READING);
  char reading= get_response();
  // if reading shows ball, throw it.
  if(is_ball(reading)){
    // wait for ball to settle
    wait(250);
    stepper_power(HIGH);
    throw_ball(get_target(reading));
    last_throw_time= millis();
    
  }
  //kill stepper when done throwing
  if (millis() > last_throw_time+1000) stepper_power(0);
  
  //check for bouncer tampering
  rest_check();
}

void throw_ball(int target){
  unsigned long start_time= millis();

  //open gate
  digitalWrite(release_pin, HIGH);
  //set interrupt to call function to close it after 500ms
  MsTimer2::set(500, release_solenoid);
  MsTimer2::start();
  
  while(millis() < start_time+300);
  goto_pos(target);
  
  // wait for ball to hit bouncer (-350ms for next ball to settle)
  while(millis() < start_time+600);
  
  //disable interrupt, ready for next use
  MsTimer2::stop();
  
  //if it's real bad, correct position
  if (needs_correction()) correct_pos();
}

void reset_sensor_thresh(int addr){ 
  send_command(addr, RESET_BALL_PRESENCE_THRESH); 
  get_response();
}  

void wait(long mills){
  if(mills<1) return;
  unsigned long start_time=millis();
  while (millis() < start_time+mills) rest_check();
} 

void dump_sensor(int addr){
  send_command(addr, DUMP_READING);
  char incoming=0;
  unsigned long start_time=millis();
  while(incoming != '\n'){
    if(Serial.available()>0){
      incoming= (Serial.read());
      Serial.print(incoming, BYTE);
    }
    if(millis()>start_time+200) break;
  }
}

int get_target(char color){
  if(color==RED) return RED_POS;
  if(color==YELLOW) return YELLOW_POS;
  if(color==BLUE) return BLUE_POS;
}

void send_command(int addr, char cmd){
  char msg[3];
  msg[0]= addr+48; //convert to ascii caracter
  msg[1]= cmd;
  msg[2]= '\0';
  
  Serial.flush(); // clear incoming buffer, expecting new stuff
  tx_msg(msg);
}

char get_response(){
  unsigned long start_time= millis();
  while(Serial.available()<2) if (millis()>start_time+100) break;
  while(Serial.read() != 'M') if (Serial.available()==0) return ERROR;
  char response= Serial.read();
  if(!is_valid(response)) response= ERROR;
  Serial.print(": ");
  Serial.println(response);
  return response;
}

boolean is_ball(char candidate){
  if(candidate == RED) return true;
  if(candidate == BLUE) return true;
  if(candidate == YELLOW) return true;
  return false;
}

boolean is_valid(char candidate){
  if(candidate == RED) return true;
  if(candidate == BLUE) return true;
  if(candidate == YELLOW) return true;
  if(candidate == OK) return true;
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



#define txPin 2

void setup_rs485_tranceiver(){
  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, LOW); // set transceiver to "receive" mode
}

void tx_msg(char *msg)
{
  digitalWrite(txPin, HIGH);
  delay(10);
  Serial.print(msg);
  delay(10);
  digitalWrite(txPin, LOW);
}

#define MIN_DELAY 100  // running delay @ spd=100
#define MAX_RUNNING_DELAY 2000 // running delay @ spd=1
#define MAX_DELAY 2000 // accel start point
#define ACCEL 50 // change in step delay per step // was 100...
#define MIN_INST_DELAY 500 // minimum instantaneous start delay

#define HOME_POS 0
#define HALF_POS 785
#define MAX_ERROR 16

#define step_pin 3
#define dir_pin 4
#define enable_pin 5
#define encoder_pin 7


long pos;
int pos_error;
long step_delay;
short step_state;
short last_encoder_state;

void setup_stepper()
{
  Serial.println("setting up stepper...");
  pos=0;
  pos_error=0;
  step_delay=MAX_DELAY;
  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);
  pinMode(enable_pin, OUTPUT);
  pinMode(encoder_pin, INPUT);
  stepper_power(HIGH);
  last_encoder_state= digitalRead(encoder_pin);
  home_stepper();
  // keep motor from coasting
  delay(50);
  stepper_power(LOW);
}

void rest_check(){
  if(last_encoder_state != digitalRead(encoder_pin)){
    int old_pos= pos;

    short ok=false;
    do{
      
      stepper_power(HIGH);
      correct_pos();
      // keep motor from coasting
      delay(50);
      stepper_power(LOW);
    
      // wait for the fucking kid to go away
      delay(3000);
      if (last_encoder_state == digitalRead(encoder_pin)) ok= true;
    }while(!ok);
    
    // go back to where it was
    stepper_power(HIGH);
    goto_pos(old_pos);
    // keep motor from coasting
    delay(50);
    stepper_power(LOW);
  }
}

void home_stepper(){
  Serial.println("homing stepper... ");
  correct_pos();
  goto_pos(0);
}

short needs_correction(){
  return (pos_error > MAX_ERROR);
}
  
void correct_pos(){
  short dir=0;
  int num_tries=0;
  int steps_taken=0;
  long wanted_pos= pos;
  Serial.print("correcting position...");
  step_delay=MIN_INST_DELAY;
  short start_encoder_state= digitalRead(encoder_pin);
  unsigned long start_time=millis();
  
  //force at least one correction
  pos_error= 100;
  
while(needs_correction()){
  
  while(digitalRead(encoder_pin) == start_encoder_state){
    pos=0;
    step(dir);
    
    steps_taken++; //try yanking it the other way
    if(steps_taken>(HALF_POS*3)/2){
      dir= !dir;
      steps_taken=0;
    }
    
    // give up after a while, for a while...
    if(millis() > start_time+2000){
      num_tries++;
      stepper_power(LOW);
      delay(1000*num_tries);
      stepper_power(HIGH);
      start_time=millis();
      // kid probably moved it while resting, so:
      start_encoder_state= digitalRead(encoder_pin);
    }
  }
  
  // assign real pos based on dir and encoder change
  if(dir){
    if(start_encoder_state) pos=HALF_POS;
    else pos= HOME_POS;
  }
  if(!dir){
    if(start_encoder_state) pos=HOME_POS;
    else pos= HALF_POS;
  }
  pos_error=0;
  
  //pass an ecoder edge to measure error
  goto_pos(pos+850);

}
  // stop motor
  delay(50);
  stepper_power(0);
  
  Serial.println("done.");
}

// this might not work and might not be neccessary anyway
short encoder_reading_consistent(){
  // normalize position (0 to full turn)
  int norm_pos=pos;
  while(norm_pos>0) norm_pos-= FULL_TURN;
  while(norm_pos<0) norm_pos+= FULL_TURN;

  // if pos is close to edge, then no tellin'
  if(abs(norm_pos-HOME_POS) < MAX_ERROR) return true;
  if(abs(norm_pos-HALF_POS) < MAX_ERROR) return true;
  
  
  if(norm_pos < HALF_POS && digitalRead(encoder_pin)) return true;
  if(norm_pos > HALF_POS && !digitalRead(encoder_pin)) return true;
  
  return false;
}
  

void goto_encoder_half(){
  step_delay=MAX_DELAY;
  while(!digitalRead(encoder_pin)) step(0);
  while(digitalRead(encoder_pin)) step(1);
}

void dump_motor_data(){
  Serial.print("pos: ");
  Serial.print(pos);
  Serial.print("  pos_error: ");
  Serial.print(pos_error);
  Serial.print("  step_delay: ");
  Serial.print(step_delay);
  Serial.print("  encoder state: ");
  if(digitalRead(encoder_pin)) Serial.println("HIGH");
  else Serial.println("LOW");
}

void goto_pos(long target){ 
  goto_pos_at_speed(target, 100);
}

void goto_pos_at_speed(long target, int spd){
  pos-= pos_error;
  pos_error=0;
  
  //make all moves be more than 1/2 turn to ensure self-checking 
  
  short dir;
  
  if (target > pos) dir=1;
  else dir=0;
  
  long dist;
  dist = abs(target-pos);
  
  //if it's too short, go the other way around
  if(dist < FULL_TURN/2){
    dir= !dir;
    if(target<pos) target+= FULL_TURN;
    else target-= FULL_TURN;
    dist= abs(target-pos);
  }
  
  long dist_left= dist;
  long change;
  int min_delay= MIN_DELAY+(((MAX_RUNNING_DELAY-MIN_DELAY)*(100-((long)spd)))/100);
  
  step_delay= MAX_DELAY;
  
  while ((dist_left > dist/2) && step_delay>min_delay){
    // take a step, start over on error
    // if (!step(dir)){ correct_pos(); goto_pos(target); return; }
    step(dir);
    dist_left--;
    change= (ACCEL*(step_delay-min_delay))/(MAX_DELAY-min_delay);  
    if(change==0) change=1;
    step_delay-=change;
  }
  int decel_steps= dist-dist_left;
  
  while(dist_left > decel_steps){
    // take a step, start over on error
    // if (!step(dir)){ correct_pos(); goto_pos(target); return; }
    step(dir);
    change= (ACCEL*(step_delay-min_delay))/(MAX_DELAY-min_delay);  
    if(change==0) change=1;
    step_delay=step_delay;
    dist_left--;
  }
  
  while(dist_left>0){
    // take a step, start over on error
    // if (!step(dir)){ correct_pos(); goto_pos(target); return; }
    step(dir);
    change= (ACCEL*(step_delay-min_delay))/(MAX_DELAY-min_delay);  
    if(change==0) change=1;
    step_delay+=change;
    dist_left--;
  }
  
  //if(abs(pos_error)>MAX_ERROR) correct_pos();
}


//takes a step, returns false if pos error detected.
short step(short dir){
  //note encoder reading.
  last_encoder_state= digitalRead(encoder_pin);
  
  //take a step...
  digitalWrite(dir_pin, dir);
  digitalWrite(step_pin, step_state);
  step_state = !step_state;
  if(step_delay<16383) delayMicroseconds(step_delay);
  else delayMicroseconds(16383);
  
  //increment pos, wrap value if necessary
  if(dir){
    pos++;
    if(pos> 1200) pos-= FULL_TURN;
  }
  else{
    pos--;
    if(pos< -400) pos+= FULL_TURN;
  }
  
  // check for encoder crossing
  if(last_encoder_state != digitalRead(encoder_pin)){
    // if crossing seen, determine pos error
    if(last_encoder_state && dir) //falling encoder, rising position
      pos_error= pos-HALF_POS;
    if(!last_encoder_state && dir) //rising encoder, rising position
      pos_error= pos-HOME_POS;
    if(last_encoder_state && !dir) //falling encoder, falling position
      pos_error= pos-HOME_POS-1;
    if(!last_encoder_state && !dir) //rising encoder, falling position
      pos_error= pos-HALF_POS-1;
  }
  
  //add: return false if expected encoder crossing not seen?
  
  if(abs(pos_error)>MAX_ERROR) return false;
  else return true;
}

void stepper_power(short enable){
  digitalWrite(enable_pin, enable);
}



int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

