#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>

SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);
#define GPSECHO  true
#define LOG_FIXONLY false  
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// Set the pins used
#define chipSelect 10
#define ledPin 13

//Neopixel
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define PIN            6
#define NUMPIXELS      3
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 500;

File logfile;
File readFile;
File logfileR;

long v11 = 559480;
long v21 = -31962;

long distanceS = 10000;
long distanceTemp = 10000;
long locX;
long locY;
long locX[50];
long locY[50];

uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {

  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

void setup() {

  Serial.begin(9600);
  Serial.println("\r\nUltimate GPSlogger Shield");
  pinMode(ledPin, OUTPUT);
  pinMode(10, OUTPUT);
  
  if (!SD.begin(chipSelect)) {   
    Serial.println("Card init. failed!");
    error(2);
  }
  logfile = SD.open("GPSLOG00.csv", FILE_WRITE);
  logfileR = SD.open("RFile.csv",FILE_WRITE);
  readFile = SD.open("Data.CSV",FILE_READ);

  
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   
  GPS.sendCommand(PGCMD_NOANTENNA);
  useInterrupt(true);

  compareVals(&locX, &locY);
  
  pixels.begin();
}

SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;  
  #endif
}

void useInterrupt(boolean v) {
  if (v) {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

uint32_t timer = millis();

void loop() {
    if (! usingInterrupt) {

    char c = GPS.read();
   
    if (GPSECHO)
      if (c) Serial.print(c);
  }

    if (GPS.newNMEAreceived()) {

    char *stringptr = GPS.lastNMEA();
    
    if (!GPS.parse(stringptr))  
      return;  
    Serial.println("Log");
    }
    
    if(GPS.fix){
      
    if (timer > millis())  timer = millis();
      if (millis() - timer > 2000) { 
        Serial.println("------gps.fix---------");
      timer = millis(); // reset the timer
      
      float lat = GPS.latitude;
      lat = degreeConv(lat);
      float latB = lat*10000;
      long latBB = (long)latB; 
      
      float lng = GPS.longitude;
      lng = degreeConv(lng);
      float lngB = lng*10000;
      long lngBB = (long)lngB;
      
      //Serial.println(lat);
      //Serial.println(lng);
  
      while(compareVals(&locX, &locY)){
        
        distanceTemp = (latBB - locX)*(latBB - locX) + (lngBB - locY)*(lngBB - locY);
        Serial.println(locX);
        
        if(distanceTemp < distanceS){
          distanceS = distanceTemp;
        }
        
      }
  
      
      logfile.print(lat, 4);logfile.print(',');
      logfile.println(-lng, 4);
      logfile.flush();
  
      logfileR.print(latBB);logfileR.print(',');
      logfileR.println(lngBB);
      logfileR.flush();
      Serial.println("-------end fix-------");
      }
    } //end of GPX.fix
    else{
      
       while(compareVals(&locX, &locY)){
      //Serial.println("not tracking!!!!");
      distanceTemp = (v11 - locX)*(v11 - locX) + (v21 - locY)*(v21 - locY);
      
      if(distanceTemp < distanceS){
        distanceS = distanceTemp;
        //Serial.println(distanceTemp);
        //displayPixel();
      } 
      
    }
    //Serial.println("------------");
    //Serial.println(distanceS);
    }

}

float degreeConv(float temp){
  int degree = temp/100;
  float minut = temp - degree*100;
  minut/=60;
  float result = degree + minut;
  return result;
}

bool readLine(File &f, char* line, size_t maxLen) {
  
  for (size_t n = 0; n < maxLen; n++) {
    int c = f.read();
    if ( c < 0 && n == 0) return false;  // EOF
    if (c < 0 || c == '\n') {
      line[n] = 0;
      return true;
    }
    line[n] = c;
  }
  
  return false; // line too long
}

bool compareVals(long* locX, long* locY) {
  char line[40], *ptr, *str;
  
  if (!readLine(readFile, line, sizeof(line))) {
    return false;  // EOF or too long
  }
  
  *locX = strtol(line, &ptr, 10);
  
  if (ptr == line) return false;  // bad number if equal
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *locY = strtol(ptr, &str, 10);
  
  return str != ptr;  // true if number found
}

void displayPixel(){

  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,150,0)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(delayval); 
}
}

