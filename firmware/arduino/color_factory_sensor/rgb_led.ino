#define b_pin 5
#define r_pin 6
#define g_pin 9

unsigned long last_update= 0;
char rgb_led_state = 'n';

void setup_rgb_led(){
  pinMode(r_pin, OUTPUT);
  pinMode(g_pin, OUTPUT);
  pinMode(b_pin, OUTPUT);
  digitalWrite(r_pin, LOW);
  digitalWrite(g_pin, LOW);
  digitalWrite(b_pin, LOW);
  
  rgb_led(255,0,0);
  delay(250);
  rgb_led(0,255,0);
  delay(250);
  rgb_led(0,0,255);
  delay(250);
  rgb_led(0,0,0);
  delay(250);
  rgb_led(255,255,255);
  delay(250);
  rgb_led(0,0,0);
  delay(250);
}

void reset_bucket_color(){
  // default colors now handled by controller
  set_current_color(NONE);
  update_rgb_led();
}

void update_rgb_led(){
  rgb_led_state= current_color;
  
  if(millis() > last_update+100){
    if(rgb_led_state ==  'o') rgb_led(255,157,0);
    if(rgb_led_state ==  'y') rgb_led(255,225,5);
    if(rgb_led_state ==  'g') rgb_led(15,255,2);
    if(rgb_led_state ==  'r') rgb_led(255,5,5);
    if(rgb_led_state ==  'b') rgb_led(5,5,255);
    if(rgb_led_state ==  'n') rgb_led(100,100,100);
    
    last_update= millis();
  }
}

void rgb_led(int r_power, int g_power, int b_power){
  analogWrite(r_pin, r_power);
  analogWrite(g_pin, g_power);
  analogWrite(b_pin, b_power);
}
