// chip: ATMEGA168-20PU 
// bootloader: Arduino Diecimila or Duemilanove w/ ATmega168
//
// This is a device that pretends to be a color sensor on the rs485 bus 
//


#include <PString.h>

#define blinkyPin 13
                         
char last_color= 'b;
char current_color= 'n';

void setup(void)
{  
  Serial.begin(9600);
  pinMode(blinkyPin, OUTPUT);
  
  setup_rs485_tranceiver();
  
  flash_led(blinkyPin);
  digitalWrite(blinkyPin, HIGH);
}

void loop() {
  communicate();  
}  

void flash_led(int pin){
  digitalWrite(pin, HIGH);
  delay(200);
  digitalWrite(pin, LOW);
  delay(200);
  digitalWrite(pin, HIGH);
  delay(200);
  digitalWrite(pin, LOW);
  delay(200);
}

void set_current_color(char color){
  current_color= color;
}
