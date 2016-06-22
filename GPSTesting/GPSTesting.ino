// link between the computer and the SoftSerial Shield
// at 9600 bps 8-N-1
// Computer is connected to Hardware UART
// SoftSerial Shield is connected to the Software UART:D2&D3

// Connect GPS to D2
// Connect sound sensor to A0
// Connect potentiometer to A1
// Connect the button to D4
// Connect LED to pin 9
  
#include <SoftwareSerial.h>
#include <SD.h>

#define BUFSZ 256                              // DO NOT CHANGE THIS SIZE
#define SOUND_SENSOR A0
#define POT A1
#define LED 9
#define BUTTON 4
#define CHIP_SELECT 10                         // For an Uno it's 10

SoftwareSerial SoftSerial(8, 7);
unsigned char buffer[BUFSZ];                   // buffer array for GPS data recieved over serial port
int count = 0;                                 // counter for buffer array
int logCount = 0;                              // number of bytes if output data
unsigned char logData[100];                    // buffer for outgoing data

// Used to debounce the button
int buttonState;                               // the current reading from the input pin
int lastButtonState = LOW;                     // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;                     // the last time the output pin was toggled
long debounceDelay = 50;                       // the debounce time; increase if the output flickers

File logfile;

//#define DEBUG

//
// Setup
//
void setup()
{
    pinMode(SOUND_SENSOR, INPUT); 
    pinMode(POT, INPUT); 
    pinMode(BUTTON, INPUT);
    pinMode(LED, OUTPUT);

    // Turn the LED off
    digitalWrite(LED, LOW);

    Serial.begin(115200);                      // the Serial port of Arduino baud rate.

    Serial.println("Setting up file on SD card");
    setupSDFile();
    
    SoftSerial.begin(9600);                    // the SoftSerial baud rate
}

//
// setupSDFile - Create and open for writing the file on the SD card
//
void setupSDFile()
{
    pinMode(10, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(CHIP_SELECT)) 
    {
        Serial.println("Card init. failed!");
        error(20);        
    }
  
    char filename[15];
    strcpy(filename, "JUNGNA00.TXT");
    for (uint8_t i = 0; i < 100; i++) 
    {
        filename[6] = '0' + i/10;
        filename[7] = '0' + i%10;
        
        // create if does not exist, do not open existing, write, sync after write
        if (!SD.exists(filename)) 
        {
            break;
        }
    }

    logfile = SD.open(filename, FILE_WRITE);
    if ( !logfile ) 
    {
        Serial.print("FAILED TO CREATE "); 
        error(10);
    }
    else
    {
        Serial.print("Writing to "); 
    }

    Serial.println(filename);
}

//
// Loop
//
void loop()
{
    boolean success = false;

#ifndef DEBUG
    if (buttonPressed())
#endif
    {
        // Location
        success = processGPSData();
        
        // Noise
        success = processSoundData() && success;
        
        // User input
        success = processPot() && success;
        
        if (success)
        {
            // Write the data to the SD card
            outputData();
        }
        
#ifdef DEBUG
        delay(2000);
#endif
    }
}

//
// Has the user given input
//
boolean buttonPressed()
{
    boolean ret = false;
    
    // read the state of the switch into a local variable:
    int reading = digitalRead(BUTTON);

    // check to see if you just pressed the button 
    // (i.e. the input went from LOW to HIGH),  and you've waited 
    // long enough since the last press to ignore any noise:  

    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState) 
    {
        // reset the debouncing timer
        lastDebounceTime = millis();
    } 
  
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // whatever the reading is at, it's been there for longer
        // than the debounce delay, so take it as the actual current state:

        // if the button state has changed:
        if (reading != buttonState) 
        {
            buttonState = reading;
    
            // only return true if the new button state is HIGH
            if (buttonState == HIGH) 
            {
                ret = true;
            }
        }
    }
  
    // save the reading.  Next time through the loop,
    // it'll be the lastButtonState:
    lastButtonState = reading;
    
    return ret;
}

//
// processGPSData
//
boolean processGPSData()
{
    boolean ret = false;

    if (SoftSerial.available())
    {
        while(SoftSerial.available() && !ret)       // reading data into char array
        {
            char c = SoftSerial.read();

            if (c == '$')
            {
                // Start of the newline 
                
#ifdef DEBUG
                Serial.write(buffer, count);        // write buffer to hardware serial port
                Serial.println();
#endif

                if (processBuffer(count) > 0)
                    ret = true;
                
                clearBufferArray();
                count = 0;                
            }
            
            buffer[count++] = c;                    // writing data into array
            if (count >= BUFSZ)
            {
                // Abandon this line
                clearBufferArray();
                count = 0;
                break;
            }
        }
    }
    
    return ret;
}

//
// Read the value from the sound sensor
//
boolean processSoundData()
{
    int sensorValue = analogRead(SOUND_SENSOR);    //use A0 to read the sound sensor

#ifdef DEBUG
    Serial.print("Sound value ");
    Serial.println(sensorValue);
#endif 

    // Write the value to the logData array
    logInt(sensorValue);
    
    logData[logCount++] = ',';
    logData[logCount++] = ' ';

    return true;
}

//
// Read the user inputted value from the potentiometer
//
boolean processPot()
{
    int sensorValue = analogRead(POT);            // use A1 to read the potentiometer

#ifdef DEBUG
    Serial.print("Potentiometer value ");
    Serial.println(sensorValue);
#endif 
    
    // Write the value to the logData array
    logInt(sensorValue);

    return true;
}

//
// outputData
//
void outputData()
{
    byte err = 0;
    
    unsigned char nl = '\n';
#ifdef DEBUG
    Serial.println("outputData");
    Serial.write(logData, logCount);
    Serial.println();
#endif
    
    // Write the data to the SD card
    if (logfile.write(logData, logCount) != logCount)
    {
        err = 4;
    }
    
    if (logfile.write(&nl, 1) != 1)
    {
        err = 4;
    }
    logfile.flush();
    logCount = 0;

    if (err)
    {
        error(err);
    }
    else
    {
        // Flash the LED to confirm logging of data
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
    }
}

/**********************************************************************************************/
/* SUPPORT FUNCTIONS                                                                          */
/**********************************************************************************************/
//
// Write an integer value to the output buffer. Assumes max value of 9999
//
void logInt(int val)
{
    boolean zero = false;

    if (val > 999)
    {
        logData[logCount++] = char('0') + val / 1000;        
        val = val % 1000;
        zero = true;
    }

    if ((val > 99) || zero)
    {
        logData[logCount++] = char('0') + val / 100;        
        val = val % 100;
        zero = true;
    }

    if ((val > 9) || zero)
    {
        logData[logCount++] = char('0') + val / 10;        
        val = val % 10;
    }
    
    logData[logCount++] = char('0') + val % 10;
}

//
// Find the nth clause in the GPS data buffer. Assumes first clause is 0!
//
int indexOfComma(int clause)
{
    int i = 0, cl = 0;
    
    while (i < count)
    {
        if (buffer[i] == ',')
        {
            // Found a new clause
            cl++;
            if (cl == clause)
            {
                // Bingo!
                return i + 1;
            }
        }
        i++;
    }
    
    return -1;
}

//
// We are only interested in $GPGGA data from the GPS
//
boolean gpgga()
{
    return ((buffer[0] == '$') && (buffer[1] == 'G') && (buffer[2] == 'P') && (buffer[3] == 'G') && 
        (buffer[4] == 'G') && (buffer[5] == 'A') && (buffer[6] == ','));
}

//
// We are only interested in $GPRMC data from the GPS
//
boolean gprmc()
{
    if (!(((buffer[0] == '$') && (buffer[1] == 'G') && (buffer[2] == 'P') && (buffer[3] == 'R') && 
        (buffer[4] == 'M') && (buffer[5] == 'C') && (buffer[6] == ','))))
    {
        return false;
    }

    // Third clause must be 'A' to indicate valid data
    int validData = indexOfComma(2);

    if (validData == -1)
    {
        return -1;
    }
    
    if (buffer[validData] == 'A')
    {
        return true;
    }

    return false;
}

//
// We've got a full line of GPS NMEA data so parse it
//
int processBuffer(int cnt)
{
    if (cnt < 44)
    {
        // Insufficient data. Flash the LED to show problem
        error(2);
        return -1;
    }
 
    boolean gga = gpgga();
    boolean rmc = gprmc();
    
    // We are only interested in GGA and RMC data
    if (gga || rmc)
    {
        int i = 0;
        int clauseOffset = 0;
        
        if (rmc)
        {
            // Need to offset the clause positions if this is RMC data
            clauseOffset = 1;
        }
        
        // Timestamp
        logData[i++] = buffer[7];                // Hours
        logData[i++] = buffer[8];
        logData[i++] = ':';
        logData[i++] = buffer[9];                // Mins
        logData[i++] = buffer[10];
        logData[i++] = ':';
        logData[i++] = buffer[11];               // Secs
        logData[i++] = buffer[12];
        logData[i++] = ',';
        logData[i++] = ' ';

        // North or South of the equator?
        int latIndex = indexOfComma(3 + clauseOffset);

        if (latIndex == -1)
        {
            return -1;
        }
        
        if (buffer[latIndex] == 'S')
        {
            // Insert a '-' as we are South of the equator
            logData[i++] = '-';
        }
        
        // Latitude
        latIndex = indexOfComma(2 + clauseOffset);

        if (latIndex == -1)
        {
            return -1;
        }
        
        logData[i++] = buffer[latIndex];
        logData[i++] = buffer[latIndex+1];
        logData[i++] = ' ';
        
        for (int j = latIndex+2; j < latIndex+9; j++)        
            logData[i++] = buffer[j];
        
        logData[i++] = '\'';
        logData[i++] = ',';
        logData[i++] = ' ';
        
        // East or West of Grenwich?
        int lngIndex = indexOfComma(5 + clauseOffset);

        if (lngIndex == -1)
        {
            return -1;
        }

        if (buffer[lngIndex] == 'W')
        {
            // Insert a '-' as we are West of the Grenwich Meridian
            logData[i++] = '-';
        }
        
        // Longitude
        lngIndex = indexOfComma(4 + clauseOffset);

        if (lngIndex == -1)
        {
            return -1;
        }
        
        logData[i++] = buffer[lngIndex];
        logData[i++] = buffer[lngIndex+1];
        logData[i++] = buffer[lngIndex+2];
        logData[i++] = ' ';
        
        for (int j = lngIndex+3; j < lngIndex+10; j++)
            logData[i++] = buffer[j];

        logData[i++] = '\'';
        logData[i++] = ',';
        logData[i++] = ' ';
        
        // Check the Fix Quality        
        int fixQual = indexOfComma(6 + clauseOffset);

        if (lngIndex == -1)
        {
            return -1;
        }

        if (buffer[fixQual] == 0)
        {
            // Invalid GPS data
            logCount = 0;
            return -1;
        }

        logCount = i;
        return i;
    }
    
    return -1;
}

//
// Flush the GPS receive buffer
//
void clearBufferArray()
{
    for (int i=0; i<count; i++)
    { 
        buffer[i] = NULL;
    }              
}

//
// There's been an error so flash the LED a number of times
//
void error(int e)
{
    for (int i = 0; i < e; i++)
    {
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
        delay(50);
    }
}
