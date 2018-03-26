//LIBRARIES
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//DEFINE LOGGER STUFF for Arduino analog pins
#define LOG_INTERVAL  1000 // mills between entries: (reduce to take more/faster data)
// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   1 // echo data to serial port

//TEMPERATURE AND HUMIDITY
#define DHTPIN            6         // Pin for DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;
// the logging file
File logfile;

//ERROR STUFF
void error(char *str){
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

byte hour = 14;
byte minute = 44;
byte second = 47;
byte day = 26;
byte month = 3;
byte year = 1;


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
    error("Card failed, or not present");
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
    error("could not create file");
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
 //following line sets the RTC to the date & time this sketch was compiled
     RTC.begin();
     RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
     //This line sets the RTC with an explicit date & time, for example to set
     //January 21, 2014 at 3am you would call:
     //RTC.adjust(DateTime(2018, 3, 26, 15, 07, 26));
  
  logfile.println("millis, stamp, datetime, temperature, humidity");    
#if ECHO_TO_SERIAL
  Serial.println("millis, stamp, datetime, temperature, humidity");
#endif //ECHO_TO_SERIAL
 
  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}

void loop(void) {
  DateTime now;
  
  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
#endif

  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
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
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
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

#if ECHO_TO_SERIAL  
  Serial.print(humReading);
#endif //ECHO_TO_SERIAL

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();
  
}


