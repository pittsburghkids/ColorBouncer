#include <Wire.h>

#define I2C_ADDRESS 0x74       // 7bit

#define REG_CAP_RED       0x06
#define REG_CAP_GREEN     0x07
#define REG_CAP_BLUE      0x08
#define REG_CAP_CLEAR     0x09

#define REG_INT_RED_LO    0x0A
#define REG_INT_RED_HI    0x0B
#define REG_INT_GREEN_LO  0x0C
#define REG_INT_GREEN_HI  0x0D
#define REG_INT_BLUE_LO   0x0E
#define REG_INT_BLUE_HI   0x0F
#define REG_INT_CLEAR_LO  0x10
#define REG_INT_CLEAR_HI  0x11

#define REG_DATA_RED_LO   0x40
#define REG_DATA_RED_HI   0x41
#define REG_DATA_GREEN_LO 0x42
#define REG_DATA_GREEN_HI 0x43
#define REG_DATA_BLUE_LO  0x44
#define REG_DATA_BLUE_HI  0x45
#define REG_DATA_CLEAR_LO 0x46
#define REG_DATA_CLEAR_HI 0x47

void setupADJD() {
  Wire.begin();
  // sensor gain setting (Avago app note 5330)
  // CAPs are 4bit (higher value will result in lower output)
  set_register(REG_CAP_RED, 0x01);
  set_register(REG_CAP_GREEN, 0x01);
  set_register(REG_CAP_BLUE, 0x01);
  set_register(REG_CAP_CLEAR, 0x01);
}

void setGains(){  
  set_gain(REG_INT_CLEAR_LO,calibrationClear);
  set_gain(REG_INT_RED_LO,calibrationRed);
  set_gain(REG_INT_GREEN_LO,calibrationGreen);
  set_gain(REG_INT_BLUE_LO,calibrationBlue);
}

void getNewCalibration(){  
  calibrationClear = getClearGain();
  calibrationRed = getRedGain();
  calibrationGreen = getGreenGain();
  calibrationBlue = getBlueGain();
}

void dumpCalibration(){  
  Serial.print("clear gain: ");
  Serial.println(calibrationClear);
  Serial.print("red gain: ");
  Serial.println(calibrationRed);
  Serial.print("green gain: ");
  Serial.println(calibrationGreen);
  Serial.print("blue gain: ");
  Serial.println(calibrationBlue);
  Serial.println();
}

int getClearGain() {
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  while (!gainFound) {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox) {
      gainFound=1;
    } 
    else {
      set_gain(REG_INT_CLEAR_LO,half);
      performMeasurement();
      int sample = get_readout(REG_DATA_CLEAR_LO);

/*
      Serial.print("[");
      Serial.print(lowerBox);
      Serial.print(",");
      Serial.print(half);
      Serial.print(",");
      Serial.print(upperBox);
      Serial.print("] -> ");
      Serial.println(sample);
*/

      if (sample>1000) {
        upperBox=half;
      } 
      else if (sample<1000) {
        lowerBox=half;
      } 
      else {
        gainFound=1;
      }
    }
  }
  return half;
}

int getRedGain() {
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  while (!gainFound) {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox) gainFound=1;
    else {
      set_gain(REG_INT_RED_LO,half);
      performMeasurement();
      int sample = 0;

      sample=get_readout(REG_DATA_RED_LO);      
      if (sample>1000) upperBox=half;
      else if (sample<1000) lowerBox=half;
      else gainFound=1;
    }
  }
  return half;
}

int getGreenGain() {
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  while (!gainFound) {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox) gainFound=1;
    else {
      set_gain(REG_INT_GREEN_LO,half);
      performMeasurement();
      int sample = 0;

      sample=get_readout(REG_DATA_GREEN_LO);      
      if (sample>1000) upperBox=half;
      else if (sample<1000) lowerBox=half;
      else gainFound=1;
    }
  }
  return half;
}

int getBlueGain() {
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  while (!gainFound) {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox) gainFound=1;
    else {
      set_gain(REG_INT_BLUE_LO,half);
      performMeasurement();
      int sample = 0;

      sample=get_readout(REG_DATA_BLUE_LO);      
      if (sample>1000) upperBox=half;
      else if (sample<1000) lowerBox=half;
      else gainFound=1;
    }
  }
  return half;
}

void performMeasurement() {
  set_register(0x00,0x01); // start sensing

  while(read_register(0x00) != 0) {
    // waiting for a result
  }
}

int get_clear(){ return get_readout(REG_DATA_CLEAR_LO);}
int get_red()  { return get_readout(REG_DATA_RED_LO);}
int get_green(){ return get_readout(REG_DATA_GREEN_LO);}
int get_blue() { return get_readout(REG_DATA_BLUE_LO);}

int get_readout(int readRegister) {
  return read_register(readRegister) + (read_register(readRegister+1)<<8);
}

void set_gain(int gainRegister, int gain) {
  if (gain <4096) {
    uint8_t hi = gain >> 8;
    uint8_t lo = gain;

    set_register(gainRegister, lo);
    set_register(gainRegister+1, hi);
  }
}

void set_register(unsigned char r, unsigned char v)
{
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}

unsigned char read_register(unsigned char r)
{
  unsigned char v;
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(r);  // register to read
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, 1); // read a byte
  while(!Wire.available()) {
    // waiting
  }
  v = Wire.read();
  return v;
}
