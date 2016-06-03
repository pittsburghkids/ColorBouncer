#define first_pwm_pin 5

short flashStates[3] = {false, false, false};

void update_bucket_lights(){
    for(int i=0; i<3; i++){
      char color= targets[i];
      if(flashStates[i] == true){
        // flash once per second, so
        // check last 3 digits of millis
        if(millis()%1000 > 499) color= NONE;
      } 
      set_bucket_light(i, color);
    }
}

void flashTarget(int bucket, short flashing){
  flashStates[bucket]= flashing;
}

void set_bucket_light(int bucket, char color){
    if(color ==  'o') set_led(bucket*3+first_pwm_pin, 255,157,0);
    else if(color ==  'y') set_led(bucket*3+first_pwm_pin, 255,225,5);
    else if(color ==  'g') set_led(bucket*3+first_pwm_pin, 15,255,2);
    else if(color ==  'r') set_led(bucket*3+first_pwm_pin, 255,5,5);
    else if(color ==  'b') set_led(bucket*3+first_pwm_pin, 5,5,255); 
    else if(color ==  NONE) set_led(bucket*3+first_pwm_pin, 0,0,0); 
    else set_led(bucket+first_pwm_pin, 0,0,0);
}

void set_led(int r_pin, int r, int g, int b){
  analogWrite(r_pin, r);
  analogWrite(r_pin+1, g);
  analogWrite(r_pin+2, b);
}

void test_bucket_lights(){
  Serial.println("testing bucket lights...");
  
  for(int i=0; i<3; i++) set_bucket_light(i, 'r');
  delay(500);
  for(int i=0; i<3; i++) set_bucket_light(i, 'g');
  delay(500);
  for(int i=0; i<3; i++) set_bucket_light(i, 'b');
  delay(500);
  
  for(int i=0; i<3; i++) set_bucket_light(i, 'n');
    
}
