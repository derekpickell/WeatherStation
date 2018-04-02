//CREATED BY DEREK PICKELL, SARAH JONES & EMILY ROSE
//LIBRARIES
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_BMP280.h>

//DEFINE LOGGER STUFF for Arduino analog pins
#define LOG_INTERVAL  1000 // mills between entries: (reduce to take more/faster data)
// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // milliseconds between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   1 // echo data to serial port

//TEMPERATURE AND HUMIDITY
#define DHTPIN            6         // Pin for DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

//ANEMOMETER 
int sensorValue = 0; //Variable stores the value direct from the analog pin
float sensorVoltage = 0; //Variable that stores the voltage (in Volts) from the anemometer being sent to the analog pin
float windSpeed = 0; // Wind speed in meters per second (m/s)


//BAROMETER
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

Adafruit_BMP280 bmp; // I2C

//REAL TIME CLOCK
RTC_DS1307 RTC; // define the Real Time Clock object

//DATA LOGGING SHIELD use digital pin 10 for the SD cs line
const int chipSelect = 10;
// the logging file
File logfile;

//ERROR STUFF
void error(char *str){
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void setup(void){
  Serial.begin(9600);
  Serial.println();
  
  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error((char *) "Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // CREATE A NEW FILE
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error((char *) "could not create file");
  }

  if (!bmp.begin()){
    Serial.println(F("Could not find BMP"));
    while(1);
  }

  //Serial of which filename will show up on SD card
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL // equals 1
    Serial.println("RTC failed");
#endif  
  }
  logfile.println("datetime, temperature (C), humidity %, wind speed (mph), pressure & altitude");    
#if ECHO_TO_SERIAL
  Serial.println("datetime, temperature (C), humidity %, wind speed (mph), pressure & altitude");
#endif //ECHO_TO_SERIAL
}

void loop(void) {
  DateTime now;
  
  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  uint32_t m = millis();
  
  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(", ");
 #if ECHO_TO_SERIAL 
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(", ");
#endif //ECHO_TO_SERIAL

//TEMPERATURE
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  int tempReading = event.temperature; 
  logfile.print(tempReading);
  logfile.print(", ");   

#if ECHO_TO_SERIAL 
  Serial.print(tempReading);
  Serial.print(", ");  
#endif //ECHO_TO_SERIAL

//HUMIDITY
  dht.humidity().getEvent(&event);
  int humReading = event.relative_humidity;   
  logfile.print(humReading);
  logfile.print(", "); 

#if ECHO_TO_SERIAL //ECHO_TO_SERIAL 
  Serial.print(humReading);
  Serial.print(", ");
#endif 

//ANEMOMETER
  sensorValue = analogRead(A1);
  sensorVoltage = sensorValue * (5.0/1023);
  if (sensorVoltage <= .2){
    windSpeed = 0; //Check if voltage is below minimum value. If so, set wind speed to zero.
  }
  else {
    windSpeed = (sensorVoltage - 0.42)*32/(2.0 - .42); //For voltages above minimum value, use the linear relationship to calculate wind speed.
  }
  
  logfile.print(windSpeed);
  logfile.print(", ");
   
#if ECHO_TO_SERIAL 
  Serial.print(windSpeed);
  Serial.print(", ");
#endif  

//BAROMETER  
  logfile.print(bmp.readPressure()*.0002953);
  logfile.print(", ");
  logfile.print(bmp.readAltitude(1013.25));
  
 #if ECHO_TO_SERIAL 
  Serial.print(bmp.readPressure()*.0002953);
  Serial.print(", ");
  Serial.print(bmp.readAltitude(1013.25));
#endif

//END OF SENSOR SECTION  
  logfile.println();
  Serial.println();

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();
  
}


