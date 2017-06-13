#include <Wire.h>
#include "Adafruit_DRV2605.h"

Adafruit_DRV2605 drv;
int LRA_AMOUNT = 4;

#define TCAADDR 0x70 //multiplexer
#define DRV2605_ADDR 0x5A
 
void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

// registers
#define FEEDBACK_CONTROL 0x1A
// Below aka FEEDBACK CONTROL
#define ERM_LRA 0x1A
#define FB_BRAKE_FACTOR 0x1A
#define LOOP_GAIN 0x1A
#define RATED_VOLTAGE 0x16
#define OD_CLAMP 0x17
#define AUTO_CAL_TIME 0x1E
// Below aka CONTROL 1
#define DRIVE_TIME 0x1B
// Below aka CONTROL 2
#define BIDIR_INPUT 0x1C
#define SAMPLE_TIME 0x1C
#define BLANKING_TIME 0x1C
#define IDISS_TIME 0x1C

#define GO 0x0C
#define STATUS 0x00

uint8_t readRegister8(uint8_t reg) {
  uint8_t x ;
   // use i2c
    Wire.beginTransmission(DRV2605_ADDR);
    Wire.write((byte)reg);
    Wire.endTransmission();
    Wire.requestFrom((byte)DRV2605_ADDR, (byte)1);
    x = Wire.read();

  //  Serial.print("$"); Serial.print(reg, HEX); 
  //  Serial.print(": 0x"); Serial.println(x, HEX);
  
  return x;
}

void writeRegister8(uint8_t reg, uint8_t val) {
   // use i2c
    Wire.beginTransmission(DRV2605_ADDR);
    Wire.write((byte)reg);
    Wire.write((byte)val);
    Wire.endTransmission();
}

void setupDrv(){

  // set auto calibration mode
  writeRegister8(DRV2605_REG_MODE, 0x07); 

  // set required registers for auto calibration

  // select LRA
  writeRegister8(ERM_LRA, readRegister8(ERM_LRA) | 0x80);
  // FB_BRAKE  recommended 2
  writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) & 0x8F);
  writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) | 0x20);
  // LOOP_GAIN recommended 2
  writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) & 0xF3);
  writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) | 0x8);
  // RATED_VOLTAGE (from example, tune lower/higher [deduce effect specs])
  writeRegister8(RATED_VOLTAGE, 0x53);
  // OD_CLAMP (from example, tune lower for safety)
  writeRegister8(OD_CLAMP, 0x89);
  // AUTO_CAL_TIME recommended 3
  writeRegister8(AUTO_CAL_TIME, readRegister8(AUTO_CAL_TIME) | 0x30);
  // BIDIR_INPUT set unidrectional
  writeRegister8(BIDIR_INPUT, readRegister8(BIDIR_INPUT) | 0x80);
  // DRIVE_TIME
  writeRegister8(DRIVE_TIME, readRegister8(DRIVE_TIME) & 0xE0);
  writeRegister8(DRIVE_TIME, readRegister8(DRIVE_TIME) | 0x14);

  // perform auto calibration
  writeRegister8(GO, 1);
}

void setup() {
  Serial.begin(9600);
  Serial.println("DRV test");
  Wire.begin();
  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);

    drv.setMode(DRV2605_MODE_INTTRIG);
    setupDrv();

    // drv.begin();
    
    // drv.selectLibrary(6);
    // drv.useLRA();
  
    // I2C trigger by sending 'go' command 
    // default, internal trigger when sending GO command
    // drv.setMode(DRV2605_MODE_INTTRIG); 
  }

  delay(2000);

  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);

    drv.selectLibrary(6);
  
    // I2C trigger by sending 'go' command 
    // default, internal trigger when sending GO command
    // drv.setMode(DRV2605_MODE_INTTRIG);
    writeRegister8(DRV2605_REG_MODE, 0x00);
  }

//  tcaselect(0);
//  drv.begin();
  
//  drv.selectLibrary(1);
//  drv.useLRA();
  
  // I2C trigger by sending 'go' command 
  // default, internal trigger when sending GO command
//  drv.setMode(DRV2605_MODE_INTTRIG); 
}

uint8_t effect = 92;

void loop() {
  // Serial.print("Effect #"); Serial.println(effect);

  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);
    Serial.print(i); Serial.print(" :"); Serial.println(readRegister8(GO));
    Serial.print(i); Serial.print(" :"); Serial.println(readRegister8(STATUS));
    
//    drv.setWaveform(0, effect);  // play effect 
//    drv.setWaveform(1, 0);       // end waveform
    drv.setWaveform(0, 1);
    drv.setWaveform(1, 0);       // end waveform
//    drv.setWaveform(1, 75);
//    drv.setWaveform(2, 0);       // end waveform

    // play the effect!
    drv.go();
    delay(60);
  }
  // set the effect to play
//  drv.setWaveform(0, effect);  // play effect 
//  drv.setWaveform(1, 0);       // end waveform

  // play the effect!
//  drv.go();

  // wait a bit
  delay(1000);

//  effect++;
//  if (effect > 117) effect = 1;
}
