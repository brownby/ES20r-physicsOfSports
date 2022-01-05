/*
  BNO08x data logger
  for STM32F405 feather

  Writes requested reports to a text file (csv)
  and allows for configuration of requested sample rate

  modified 28 September 2021
  by J. Evan Smith

  Active Learning Labs, Electrical Engineering
  https://www.seas.harvard.edu/active-learning-labs
*/

/* ! DEPENDENCIES ! */ 

#include <Adafruit_BNO08x.h>
//#include <Adafruit_NeoPixel.h>
//#include <STM32SD.h>

// STM32SD depends on FatFs
// find JSON here: 
// https://github.com/stm32duino/BoardManagerFiles/raw/master/package_stmicroelectronics_index.json

// also STM32CubeProgrammer is required for upload
// https://www.st.com/en/development-tools/stm32cubeprog.html

#define BNO08X_RESET -1
//#define nPIN 8           // reference = STM32F405 feather pinout
//#define nPIX 1           // number of neopixels
#define sRate 100        // requested rate, in Hz
#define uSeconds 1000000
#define mSwitch A0       // slide switch to control state (standby or recording)
#define hSwitch A1       // high side for switch
#define cButton 5        // button to initiate calibration (?) in standby // TODO
#define hButton 9        // high side for button

#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN SD_DETECT_NONE
#endif

Adafruit_BNO08x bno08x(BNO08X_RESET);
//Adafruit_NeoPixel pixels(nPIX,nPIN, NEO_GRB + NEO_KHZ800);
sh2_SensorValue_t sensorValue;

char dataBuffer[40];

bool recording = false;
unsigned long start_time;

File dataFile;

void setup(void) {
  Serial.begin(115200);
  // while(!Serial){delay(10);} // add or remove comment here to deploy or debug, respectively
//  pixels.begin(); pixels.clear();
//  pixels.setPixelColor(0, pixels.Color(0, 50, 0)); 
//  pixels.show();  

  if (!bno08x.begin_I2C()) {
//    pixels.clear();
//    pixels.setPixelColor(0, pixels.Color(50, 20, 0)); pixels.show();  
    Serial.println("Failed to find BNO08x chip (!)");
    while (!bno08x.begin_I2C()) {
      Serial.println("Trying again ... ");
      delay(250);
    }
  }
//  pixels.clear();
//  pixels.setPixelColor(0, pixels.Color(0, 50, 0)); pixels.show();  
  Serial.println("BNO08x found (+)");

  Serial.println("Initializing SD card ... ");
  while (!SD.begin(SD_DETECT_PIN)){
//    pixels.clear();
//    pixels.setPixelColor(0, pixels.Color(50, 20, 0)); pixels.show();  
    delay(10);
  }
  Serial.println("SD card initialized (+)");

  pinMode(mSwitch,INPUT_PULLUP);
  pinMode(hSwitch,INPUT_PULLUP);
  pinMode(cButton,INPUT_PULLUP);
  pinMode(hButton,INPUT_PULLUP);
  pinMode(LED_BUILTIN,OUTPUT);

  digitalWrite(hSwitch,HIGH);
  digitalWrite(hButton,HIGH);
  digitalWrite(LED_BUILTIN,HIGH);

  bnoDetails();
  setReports();

  Serial.print("Sample rate configured to: "); // requested, will vary
  Serial.print(sRate);
  Serial.println(" Hz"); 
//  pixels.clear();
//  pixels.setPixelColor(0, pixels.Color(0, 50, 0)); pixels.show();  
  delay(100);
  digitalWrite(LED_BUILTIN,LOW);
}

void bnoDetails(void) {
  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }
}

void setReports(void) {
  Serial.println("Setting desired reports ... ");
  if (!bno08x.enableReport(SH2_ACCELEROMETER,uSeconds/sRate)) {
    Serial.println("Could not enable accelerometer (!)");
  }
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED,uSeconds/sRate)) {
    Serial.println("Could not enable gyroscope (!)");
  } /* // disabled begin
  if (!bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED,uSeconds/sRate)) {
    Serial.println("Could not enable magnetic field calibrated (!)");
  }
  if (!bno08x.enableReport(SH2_LINEAR_ACCELERATION)) {
    Serial.println("Could not enable linear acceleration");
  }
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
  if (!bno08x.enableReport(SH2_RAW_ACCELEROMETER,100000)) {
    Serial.println("Could not enable raw accelerometer");
  }
  if (!bno08x.enableReport(SH2_RAW_GYROSCOPE,100000)) {
    Serial.println("Could not enable raw gyroscope");
  }
  if (!bno08x.enableReport(SH2_RAW_MAGNETOMETER,100000)) {
    Serial.println("Could not enable raw magnetometer");
  }
  */ // disabled end
  Serial.println("Reports set (+)");
  Serial.println("In standby ...");
}

void print_uint64_t(uint64_t num) {
  char rev[128]; 
  char *p = rev+1;
  while (num > 0) {
    *p++ = '0' + ( num % 10);
    num/= 10;
  }
  p--;
  /*Print the number which is now in reverse*/
  while (p > rev) {
    Serial.print(*p--);
  }
}

void loop() {
  
  delayMicroseconds(10); // TODO
  
  if (bno08x.wasReset()) {
    Serial.println("Sensor was reset ...");
    Serial.println("In standby ...");
    setReports();
  }

  if (!digitalRead(mSwitch)){
    if (!recording){
      recording = !recording;
      dataFile = SD.open("datalog.txt", FILE_WRITE);
      Serial.println("Recording (!)");
//      pixels.clear();
//      pixels.setPixelColor(0, pixels.Color(50, 0, 0)); pixels.show();  
      start_time = micros();
    }
    
    if (!bno08x.getSensorEvent(&sensorValue)) { return; }

    switch (sensorValue.sensorId) {
      case SH2_ACCELEROMETER:
        sprintf(dataBuffer, "a,%4.2f,%4.2f,%4.2f,%u", 
                sensorValue.un.accelerometer.x, sensorValue.un.accelerometer.y, 
                sensorValue.un.accelerometer.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_GYROSCOPE_CALIBRATED:
        sprintf(dataBuffer, "g,%4.2f,%4.2f,%4.2f,%u", 
                sensorValue.un.gyroscope.x, sensorValue.un.gyroscope.y, 
                sensorValue.un.gyroscope.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_MAGNETIC_FIELD_CALIBRATED:
        sprintf(dataBuffer, "m,%4.2f,%4.2f,%4.2f,%u", 
                sensorValue.un.magneticField.x, sensorValue.un.magneticField.y, 
                sensorValue.un.magneticField.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_LINEAR_ACCELERATION:
        sprintf(dataBuffer, "la,%4.2f,%4.2f,%4.2f,%u", 
                sensorValue.un.linearAcceleration.x, sensorValue.un.linearAcceleration.y, 
                sensorValue.un.linearAcceleration.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_ROTATION_VECTOR:
        sprintf(dataBuffer, "rv,%4.2f,%4.2f,%4.2f,%4.2f,%u", 
                sensorValue.un.rotationVector.real, sensorValue.un.rotationVector.i, 
                sensorValue.un.rotationVector.j, sensorValue.un.rotationVector.k, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_RAW_ACCELEROMETER:
        sprintf(dataBuffer, "ra,%4.2d,%4.2d,%4.2d,%u", 
                sensorValue.un.rawAccelerometer.x, sensorValue.un.rawAccelerometer.y, 
                sensorValue.un.rawAccelerometer.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_RAW_GYROSCOPE:
        sprintf(dataBuffer, "rg,%4.2d,%4.2d,%4.2d,%u", 
                sensorValue.un.rawGyroscope.x, sensorValue.un.rawGyroscope.y, 
                sensorValue.un.rawGyroscope.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      case SH2_RAW_MAGNETOMETER:
        sprintf(dataBuffer, "rg,%4.2d,%4.2d,%4.2d,%u", 
                sensorValue.un.rawMagnetometer.x, sensorValue.un.rawMagnetometer.y, 
                sensorValue.un.rawMagnetometer.z, (micros() - start_time));
        dataFile.println(dataBuffer);  
        break;
      default:
        break;
    }
  }
  else{
    if (recording) {
      recording = !recording;
      dataFile.close();
      Serial.println("In standby ...");
//      pixels.clear();
//      pixels.setPixelColor(0, pixels.Color(0, 50, 0)); pixels.show();  
    }
  }
}
