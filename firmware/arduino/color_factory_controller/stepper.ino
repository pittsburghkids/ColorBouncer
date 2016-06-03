#define MIN_DELAY 275  // minimum running delay
#define MAX_DELAY 2250 // accel start point
#define ACCEL 50 // change in step delay per step // was 50...
#define MIN_INST_DELAY 2000 // minimum instantaneous start delay

#define ZERO_POS 0
#define HALF_POS 785
#define MAX_POS_ERROR 5
#define HOME_POS -305

#define step_pin A3
#define dir_pin A2
#define enable_pin A1

#define enc_A_pin 2
#define enc_B_pin 3
#define enc_I_pin 4

volatile long enc_pos;
volatile long pos;

long step_delay;

void setup_stepper(int start_pos)
{
  Serial.println("setting up stepper...");
  
  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);
  pinMode(enable_pin, OUTPUT);
  
  pinMode(enc_A_pin, INPUT);
  pinMode(enc_B_pin, INPUT);
  pinMode(enc_I_pin, INPUT);
  // interrupt on encoder A pin (pin 2) rise
  attachInterrupt(0, update_enc, CHANGE);

  calibrateStepper(start_pos);
}

void calibrateStepper(int start_pos){
  Serial.println("calibrating stepper position...");
  // spin motor at least one rev. to calibrate encoder
  short done= false;
  while(!done){
    // spin a rev to catch index pulse
    goto_pos(enc_pos+FULL_TURN);
    // remove momentum
    delay(200); 
    
    // test your work
    for(int i=-1*MAX_POS_ERROR/2; i<MAX_POS_ERROR/2; i++){
      goto_pos(HOME_POS+i);
      delay(100);
      if(digitalRead(enc_I_pin)) done=true;
    }
    goto_pos(start_pos);
  }
}

void update_enc(){
  // this only runs on A pin change, so B pin state should give direction
  // there are 4 drive steps per encoder A state change
  // so multiply to match motor drive steps
  if(digitalRead(enc_A_pin) == digitalRead(enc_B_pin)) enc_pos+= 4;
  else enc_pos-= 4;
}

void dump_motor_data(){
  Serial.print("pos: ");
  Serial.println(pos);
  Serial.print("enc_pos: ");
  Serial.println(enc_pos);
  Serial.print("error: ");
  Serial.println(pos_error());
  
  
  //Serial.print("  pos_error: ");
  //Serial.print(pos_error);
  //Serial.print("  step_delay: ");
  //Serial.println(step_delay);
  Serial.println();
}

void goto_pos(int target){
  
  // movements now based on enc_pos instead of pos.
  short dir= target > enc_pos;
  int remaining_steps= abs(target-enc_pos);
  
  int decel_steps= 0;
  int decel_start= 0;
  long change;
  
  step_delay= MAX_DELAY;
  
  // Accelerate until either max speed reached or halfway to target...
  while (step_delay > MIN_DELAY && remaining_steps > decel_steps){
    take_step(dir);
    decel_steps++;
    remaining_steps--;
    change= (ACCEL*(step_delay-MIN_DELAY))/(MAX_DELAY-MIN_DELAY);  
    if(change==0) change=1;
    step_delay-= change;
  }
    
  // spin to decel start point
  while(remaining_steps > decel_steps){
    take_step(dir);
    remaining_steps--;
  }
    
  // decell to target
  while(remaining_steps > 0){
    take_step(dir);
    remaining_steps--;
    change= (ACCEL*(step_delay-MIN_DELAY))/(MAX_DELAY-MIN_DELAY);  
    if(change==0) change=1;
    step_delay+= change;
  }
  
  while(abs(pos_error()) > MAX_POS_ERROR){
    delay(250);
    pos= enc_pos;
    goto_pos(target);
  }
}
    
int pos_error(){
  return enc_pos-pos;
} 

void take_step (short dir){
//take a step, checking for index pulse
  digitalWrite(dir_pin, dir);
  delayMicroseconds(1); // direction setup as per driver manual
  digitalWrite(step_pin, HIGH);
  
  // delayMicroseconds is confused by delays over 16383
  if(step_delay<16383){
    delayMicroseconds(3); // 3 * min step pulse time as per driver manual
    digitalWrite(step_pin, LOW);
    delayMicroseconds(step_delay);
  }
  else{
    delayMicroseconds(3);
    digitalWrite(step_pin, LOW);
    delayMicroseconds(16383);
  } 
  
  // increment pos
  if(dir) pos++;
  else pos--;
  
  // if index crossed, calibrate home
  // I don't know how wide the index pulse is,
  // so it might be necessary to restrict this to one dir
  if (digitalRead(enc_I_pin)){ pos= HOME_POS; enc_pos= HOME_POS; }
}

void tamper_check(){
  
  // if the bouncer has been forced out of position, move it back
  while(abs(pos_error()) > MAX_POS_ERROR){
    delay(250);
    int target= pos;
    pos= enc_pos;
    goto_pos(target);
  }
}

void stepper_power(short enable){
  // logic high disables drive
  digitalWrite(enable_pin, !enable);
}

short motor_is_at(long p){
// test if given pos is "equal enough" to current motor pos
  return (abs(enc_pos - p) < MAX_POS_ERROR); 
}
