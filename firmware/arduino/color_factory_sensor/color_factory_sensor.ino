// chip: ATMEGA168-20PU 
// bootloader: 
//

#include <PString.h>

// ball colors: red, green, yellow, blue, purple, orange

#define DUMP_READINGS 0
#define MAX_COLOR_ERROR 7

#define RED 'r'
#define GREEN 'g'
#define BLUE 'b'
#define YELLOW 'y'
#define ORANGE 'o'
#define NONE 'n'

#define sensor_ledPin 3
#define blinkyPin 13

// no orange, so...
#define NUM_BALL_COLORS 4 // this should be <= defined ball_colors[x][]

// address of this sensor on the rs485 bus
// read from stored value in EEPROM
char address;

byte ball_colors[][4]= {  {64, 22, 13, RED},  //  red
                          {29, 42, 28, GREEN},  //  green
                          {24, 28, 46, BLUE},  //  blue        
                          {45, 38, 15, YELLOW},  //  yellow
                          { 0,  0,  0, ORANGE} };// orange
                         
// vars for holding calibration data (read from eeprom?)
unsigned int calibrationClear = 960;
unsigned int calibrationRed = 2720;
unsigned int calibrationGreen = 2064;
unsigned int calibrationBlue = 2928;
unsigned int c,r,g,b;
char last_color= RED;
char current_color= NONE;

// ball presence threshold ("clear" reading) 
// (unsigned) byte as it is a single byte read from EEPROM
byte ball_presence_thresh;

unsigned long last_ball_time= millis();


void setup(void)
{  
  Serial.begin(9600);
  pinMode(sensor_ledPin, OUTPUT);
  pinMode(blinkyPin, OUTPUT);
  
  setup_rs485_tranceiver();
  setup_rgb_led();
  
  flash_led(blinkyPin);
  digitalWrite(blinkyPin, HIGH);
  
  flash_led(sensor_ledPin);
  digitalWrite(sensor_ledPin, HIGH);  
  
  setupADJD();
  
  //address= '1';
  //write_address_to_EEPROM();
  get_address_from_EEPROM();

  //ball_presence_thresh= 100;
  //write_ball_presence_thresh_to_EEPROM();
  get_ball_presence_thresh_from_EEPROM();

  //getNewCalibration();
  //writeCalibrationToEEPROM();
  //while(1);
  
  getCalibrationFromEEPROM();  
  dumpCalibration();
  setGains();
  
  //write_ball_colors_to_EEPROM();
  get_ball_colors_from_EEPROM();

  reset_bucket_color();
}

void loop() {
  update_measurements();
  update_current_color();
  communicate();
  update_rgb_led();
  
  if(DUMP_READINGS) dump_reading();
  if(DUMP_READINGS) delay(1000);
}  

void update_current_color(){  
  char new_color= find_color();
  if(new_color != 'n'){
    current_color= new_color;
    last_ball_time= millis();
  }
  else if(millis() > last_ball_time+500) reset_bucket_color();
}

void set_current_color(char color){
  current_color= color;
} 

void update_measurements(){
  performMeasurement();
  c= get_clear();
  r= get_red();
  g= get_green();
  b= get_blue();
}

char find_color(){
  if (c < ball_presence_thresh) return 'n';
  
  //cut measurements in half to avoid overflow when finding percentages
  unsigned int hr=r/2;
  unsigned int hg=g/2;
  unsigned int hb=b/2;
  
  int percents[3]= {  (hr*100)/(hr+hg+hb), 
                      (hg*100)/(hr+hg+hb),
                      (hb*100)/(hr+hg+hb)  };
  
  // make vars for %r, %g, %b- then test against optimal values for each color
  // r: 100,  0, 0 - o: 100, 50, 0 - etc.
  // add up differences between measured and optimal for r,g,&b.
  
  int errors[NUM_BALL_COLORS]; // store relative errors for each possible color
  for(int i=0; i<NUM_BALL_COLORS; i++){
    errors[i]=0;
    for(int j=0; j<3; j++){  // remember: ball_colors[i][3] is for color name char
      errors[i] += abs(ball_colors[i][j] - percents[j]);
    }
  } 
  
  int min_err= 100;
  byte best_color;
  for(int i=0; i<NUM_BALL_COLORS; i++){
    if (errors[i] < min_err){
      min_err= errors[i];
      best_color= ball_colors[i][3]; // get color name character
    }
  }
  
  if(min_err <= MAX_COLOR_ERROR) return best_color;
  else return 'n';
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

void dump_reading(){
  int dr= ((r/2)*100) / ((r/2)+(g/2)+(b/2)); // dump red
  int dg= ((g/2)*100) / ((r/2)+(g/2)+(b/2)); // ...
  int db= ((b/2)*100) / ((r/2)+(g/2)+(b/2));
  
  int ci= -1; // color index
  if(find_color() == 'r') ci= 0;
  if(find_color() == 'g') ci= 1;
  if(find_color() == 'b') ci= 2;
  
  int er= -1;
  if(ci != -1) er= abs(dr-ball_colors[ci][0]) + abs(dg-ball_colors[ci][1]) + abs(db-ball_colors[ci][2]);
  
  Serial.print("c: ");
  Serial.print(c);
  Serial.print(", r%: ");
  Serial.print(dr);
  Serial.print(", g%: ");
  Serial.print(dg);
  Serial.print(", b%: ");
  Serial.print(db);
  Serial.print(", read color: ");
  Serial.print(find_color());
  Serial.print(", error: ");
  Serial.print(er);
  Serial.print(", current_color: ");
  Serial.println(current_color);
}

