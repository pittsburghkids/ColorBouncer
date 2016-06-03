/////////////////////////////////////////
//  eeprom memory use:                 //
//  --------------------------         //
//  0 - 3: calibration data            //
//  4 - ?: ball color rgb data         //
//    (color name chars not included)  //
//  25: ball presence threshold        //
//  26: sensor bus address             //
/////////////////////////////////////////

#include <EEPROM.h>

void getCalibrationFromEEPROM() {
  calibrationClear = 16*EEPROM.read(0);
  calibrationRed = 16*EEPROM.read(1);
  calibrationGreen = 16*EEPROM.read(2);
  calibrationBlue = 16*EEPROM.read(3);
  Serial.println("Read Calibration from EEPROM:");
  Serial.print("clear = ");
  Serial.println(calibrationClear);
  Serial.print("red = ");
  Serial.println(calibrationRed);
  Serial.print("green = ");
  Serial.println(calibrationGreen);
  Serial.print("blue = ");
  Serial.println(calibrationBlue);
}

void writeCalibrationToEEPROM() {
  EEPROM.write(0,calibrationClear/16);
  EEPROM.write(1,calibrationRed/16);
  EEPROM.write(2,calibrationGreen/16);
  EEPROM.write(3,calibrationBlue/16);
  Serial.println("Calibration written to EEPROM");
}

void get_ball_presence_thresh_from_EEPROM(){
  Serial.print("reading ball presence threshold preset from EEPROM: ");
  ball_presence_thresh= EEPROM.read(25);
  Serial.println(ball_presence_thresh);
}

void write_ball_presence_thresh_to_EEPROM(){
  EEPROM.write(25, ball_presence_thresh);
  Serial.println("ball presence threshold written to EEPROM");
}

void get_address_from_EEPROM(){
  Serial.print("reading address from EEPROM: ");
  address= EEPROM.read(26);
  Serial.println(address);
}

void write_address_to_EEPROM(){
  EEPROM.write(26, address);
  Serial.println("address written to EEPROM");
}

void get_ball_colors_from_EEPROM(){
  Serial.println("reading ball colors from EEPROM:");
  int addr= 4; // see table above
  for(int color=0; color < NUM_BALL_COLORS; color++){
    Serial.print(char(ball_colors[color][3]));
    Serial.print(": ");
    
    for(int i=0; i<3; i++){
      ball_colors[color][i]= EEPROM.read(addr);
      addr++;
      
      Serial.print(ball_colors[color][i]);
      Serial.print(", ");
    }
    Serial.println();
  }
  Serial.println();
}

void write_ball_colors_to_EEPROM(){
  int addr= 4;
  for(int color=0; color < NUM_BALL_COLORS; color++){
    for(int i=0; i<3; i++){
      EEPROM.write(addr, ball_colors[color][i]);
      addr++;
    }
  }
  Serial.println("Ball colors written to EEPROM.");
}


