#include <Wire.h>
#include "Adafruit_DRV2605.h"

#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEGatt.h"

#include "BluefruitConfig.h"

Adafruit_DRV2605 drv;
int LRA_AMOUNT = 4;

int32_t serviceId;
int32_t characteristicId;

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

// bluetooth
#define MODE_LED_BEHAVIOUR    "MODE"


/* use hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

Adafruit_BLEGatt gatt(ble);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup() {
  boolean success;

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


   /* Initialise BLE module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in command mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  /* Perform a factory reset to make sure everything is in a known state */
  Serial.println(F("Performing a factory reset: "));
  if (! ble.factoryReset() ){
       error(F("Couldn't factory reset"));
  }

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Tactile Transfer': "));

  if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Tactile Transfer")) ) {
    error(F("Could not set device name?"));
  }

  Serial.println(F("Adding custom service definition : "));
  success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID128=e7-53-a5-9e-00-f0-41-73-96-08-b4-5f-d8-79-c8-48"), &serviceId);
  if (! success) {
    error(F("Could not add service"));
  }

  Serial.println(F("Adding custom characteristic definition : "));
  // PROPERTIES = read + write + notify
  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID128=12-f2-77-e3-79-15-4d-a9-be-bc-60-71-ef-2c-e2-e7,PROPERTIES=0x1A,MIN_LEN=1,DATATYPE=INTEGER,VALUE=0"), &characteristicId);
  if (! success) {
    error(F("Could not add characteristic"));
  }

  Serial.println(F("Setting advertising data : "));
  // first the flags for the device(02-01-06), then advertise service UUID
  success = ble.sendCommandCheckOK((F("AT+GAPSETADVDATA=02-01-06-11-07-48-c8-79-d8-5f-b4-08-96-73-41-f0-00-9e-a5-53-e7")));
  if (!success) {
    error(F("Could not set advertising data"));
  }

  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing ble SW reset (service changes require a reset): "));
  ble.reset();



  // TODO: replace with func to check that all GO bits are cleared
  delay(3000);
  
  for (int i = 0; i < LRA_AMOUNT; i++){
    tcaselect(i);

    drv.selectLibrary(6);
  
    // set mode to standby
    writeRegister8(DRV2605_REG_MODE, 0x00);
  }

  // wait for connection
  while (! ble.isConnected()) {
    delay(500);
  }

  // Change Mode LED Activity
  Serial.println(F("******************************"));
  Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
  ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  Serial.println(F("******************************"));

}


void loop() {
  int p;
  boolean success;
  // Check for user input
  // char inputs[BUFSIZE+1];

  // if ( getUserInput(inputs, BUFSIZE) )
  // {
  //   // Send characters to Bluefruit
  //   Serial.print("[Send] ");
  //   Serial.println(inputs);

  //   ble.print("AT+BLEUARTTX=");
  //   ble.println(inputs);

  //   // check response stastus
  //   if (! ble.waitForOK() ) {
  //     Serial.println(F("Failed to send?"));
  //   }
  // }
  // success = ble.sendCommandWithIntReply(F("AT+GATTCHAR=" characteristicId), &p)
  // if (!success) {
  //   error(F("Could not get characteristic data"));
  // }

  ble.print("AT+GATTCHAR=");
  ble.println(characteristicId);
  
  while (ble.readline())
  {
    Serial.print(ble.buffer);
    if ( strcmp(ble.buffer, "OK") == 0 )
    {
    }
    else if ( strcmp(ble.buffer, "ERROR") == 0 )
    {
    } else {
      p = atoi(ble.buffer);
    }
  }

  if (p == 0) {
    return;
  }

  doPattern(p);
  ble.print("AT+GATTCHAR=");
  ble.print(characteristicId);
  ble.println(",0");
  ble.waitForOK();

  // switch (p) {
  //   case 1:
  //     doPattern(1);
  //     ble.print("AT+GATTCHAR=");
  //     ble.print(characteristicId);
  //     ble.println(",0");
  //     ble.waitForOK();
  //     // success = ble.sendCommandCheckOK(F("AT+GATTCHAR=" characteristicId ",0"))
  //     // if (!success) {
  //     //   error(F("Could not set characteristic data"));
  //     // }
  //     break;
  //   case 2:
  //     doPattern(2);
  //     ble.print("AT+GATTCHAR=");
  //     ble.print(characteristicId);
  //     ble.println(",0");
  //     ble.waitForOK();
  //     // success = ble.sendCommandCheckOK(F("AT+GATTCHAR=" characteristicId ",0"))
  //     // if (!success) {
  //     //   error(F("Could not set characteristic data"));
  //     // }
  //     break;
  //   case 3:
  //     doPattern(3);
  //     ble.print("AT+GATTCHAR=");
  //     ble.print(characteristicId);
  //     ble.println(",0");
  //     ble.waitForOK();
  //     // success = ble.sendCommandCheckOK(F("AT+GATTCHAR=" characteristicId ",0"))
  //     // if (!success) {
  //     //   error(F("Could not set characteristic data"));
  //     // }
  //     break;
  //   case 0:
  //     return;
  // }

  // Check for incoming characters from Bluefruit
  // ble.println("AT+BLEUARTRX");
  // ble.readline();
  // if (strcmp(ble.buffer, "OK") == 0) {
  //   // no data
  //   return;
  // }
  // // Some data was found, its in the buffer
  // Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  // // ble.waitForOK();

  // doPattern(ble.buffer);
  // ble.waitForOK();

  // check status of patterns (GO bit). skip applying any patterns if any aren't done?

  // apply pending patterns

}


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

void doPattern(int p) {
  
  switch(p) {
    case 1:
      for (int i = 0; i < LRA_AMOUNT; i++){
        tcaselect(i);

        drv.setWaveform(0, 1);
        drv.setWaveform(1, 0);
        drv.go();
      }

      delay(100);

      for (int i = 0; i < LRA_AMOUNT; i++){
        tcaselect(i);

        drv.setWaveform(0, 1);
        drv.setWaveform(1, 0);
        drv.go();
      }
      break;
    case 2:
      tcaselect(0);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();

      tcaselect(3);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();

      delay(80);

      tcaselect(1);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();

      tcaselect(2);
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();
      break;
    case 3:
      for (int i = LRA_AMOUNT; i > 0; i--){
        tcaselect(i);

        drv.setWaveform(0, 1);
        drv.setWaveform(1, 0);
        drv.go();
        delay(60);
      }
      
      break;
    case 4:
      for (int i = 0; i < LRA_AMOUNT; i++){
        tcaselect(i);

        drv.setWaveform(0, 1);
        drv.setWaveform(1, 0);
        drv.go();
        delay(60);
      }
      
      break;
  }
  

}

void doWaveform(int p) {
  drv.setWaveform(0, p);
  drv.setWaveform(1, 0);
  drv.go();
}


/**************************************************************************/
/*!
    @brief  Checks for user input (via the Serial Monitor)
*/
/**************************************************************************/
bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while( (!Serial.available()) && !timeout.expired() ) { delay(1); }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count=0;
  do
  {
    count += Serial.readBytes(buffer+count, maxSize);
    delay(2);
  } while( (count < maxSize) && (Serial.available()) );

  return true;
}
