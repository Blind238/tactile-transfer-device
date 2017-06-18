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
  // writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) & 0x8F);
  // writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) | 0x20);
  // FB_BRAKE  dafault 3
  writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) & 0x8F);
  writeRegister8(FB_BRAKE_FACTOR, readRegister8(FB_BRAKE_FACTOR) | 0x30);
  // LOOP_GAIN recommended 2
  // writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) & 0xF3);
  // writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) | 0x8);
  // LOOP_GAIN default 1
  writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) & 0xF3);
  writeRegister8(LOOP_GAIN, readRegister8(LOOP_GAIN) | 0x4);
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
  // writeRegister8(DRIVE_TIME, readRegister8(DRIVE_TIME) | 0x14);
  writeRegister8(DRIVE_TIME, readRegister8(DRIVE_TIME) | 0x16);

  // LRA_DRIVE_MODE set to 1
  // writeRegister8(0x1D, readRegister8(0x1D) | 0x4);

  // perform auto calibration
  writeRegister8(GO, 1);
}

void setupDrvAda() {
  drv.begin();
  drv.useLRA();

  // I2C trigger by sending 'go' command 
  // default, internal trigger when sending GO command
  drv.setMode(DRV2605_MODE_INTTRIG); 
}

void setupDrvNoCalibration() {
    writeRegister8(ERM_LRA, readRegister8(ERM_LRA) | 0x80);
    writeRegister8(BIDIR_INPUT, readRegister8(BIDIR_INPUT) | 0x80);

}

void setup() {
  Serial.begin(9600);
  Serial.println("DRV test");
  Wire.begin();
  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);

    drv.setMode(DRV2605_MODE_INTTRIG);
    
    // setupDrvAda();
    setupDrv();
    // setupDrvNoCalibration();
  }

  delay(4000);

  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);

    drv.selectLibrary(6);
  
    // set mode to standby
    writeRegister8(DRV2605_REG_MODE, 0x00);
  }

}

uint8_t effect = 92;

void doPattern(int p) {
  switch (p) {
    case 1: // IMPACT
      tcaselect(0);
      drv.setWaveform(0, 4);
      drv.setWaveform(1, 0);
      drv.go();

      tcaselect(3);
      drv.setWaveform(0, 4);
      drv.setWaveform(1, 0);
      drv.go();

      delay(60);

      tcaselect(1);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();

      tcaselect(2);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();
      break;
  }
}



void loop() {
  // import/implement bluetooth lib

  // check if there are any commands/interactions from bluetooth

  // check status of patterns (GO bit). skip applying any patterns if any aren't done?

  // apply pending patterns

  doPattern(1);

  // for (int i = 0; i < LRA_AMOUNT; i++){
  //   tcaselect(i);
  //   Serial.print(i); Serial.print(" GO   :"); Serial.println(readRegister8(GO));
  //   Serial.print(i); Serial.print(" DIAG :"); Serial.println(bitRead(readRegister8(STATUS), 3));
    

  //   drv.setWaveform(0, 1); //strong click 100%
  //   // drv.setWaveform(0, 4); //sharp click 100%
  //   drv.setWaveform(1, 0); // end waveform

  //   // play the effect!
  //   drv.go();
  //   delay(50);
  // }
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
