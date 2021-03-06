/********************************************************
****** Wind Logger based upon the:  *********************
****** DataDuino - Arduino DAQ UNIT *********************
****** by Matt Little ***********************************
****** Date: 29/1/2014 **********************************
****** info@re-innovation.co.uk *************************
****** www.re-innovation.co.uk **************************
*********************************************************

  See www.re-innovation.co.uk for information and construction details

  This is sample code for the DataDuino.

  Details of Code

  The DataDuino is an Arduino based SD card datalogger.
  A PCF8563 Realt Time Clock is used to timestamp the data.
  
  Pin D3 is set up to cound pulses from a sensor (such as a anemometer or flow sensor)
  These are pulses are averaged into a wind speed
  
  Pin A0 is set up to read a thermistor for temperature (GND-47k thermistor - 47k -Vref)
  
  Pin A1 is set up with a 47k/47k potential divider from the input voltage
  
  Pin A2 is set up ro measure wind direction with a 10k resistor to GND (GND-10k-R vane-Vref)
  
  Pin D4 is set up to switch on the power to some IR sensors (for wind direction)
  
  *Pin A1,A2,A3 are set to digital and return data about wind vane direction (NOT IMPLEMENTED YET)
  
  Pin D7 is used as an input to choose calibrate mode
  When in calibrate mode the serial port is read and sample/time/date can be updated
  
  Each logger has a reference (user adjustable).
  
  Data is written to a .csv file created on an SD card.
  A new file is created each day. If file alreay present then data is appended.
  The file name is created from the reference number and the date in the format:
  DXXXXXX.csv, where DXXXXXX is the date in the format DDMMYY. 
  
  Data is stored with human readable headers:
  "Reference, Time, Date, Wind Pulses, Direction, Temp, Humidity"

  // This will be added to a seperate 'calibrate' mode
  When in Calibrate mode:
  You can adjust the parameters of the device using serial commands. These parameters are stored in EEPROM.
  These are:
  T??????E
  This will change the time to HHMMSS
  D??????E
  This will change the date to DDMMYY
  S?????E
  This will change the sample period to ????? seconds. Set to 00001 for 1 second data, set to 03600 for 1 hour data.
  The minimum is 1 second data. The maximum is 99999 seconds
  
  // Addedd Interrupt code from here:
  // PinChangeIntExample, version 1.1 Sun Jan 15 06:24:19 CST 2012
  // See the Wiki at http://code.google.com/p/arduino-pinchangeint/wiki 
  // for more information.
 
  Updates: 
  30/10/12 Code written - Matt Little
  31/10/12 writing code description and sorting out new sample timings - Matt Little
  19/9/13  Getting code to work on Arduino 1.0.5 - Matt Little
  5/2/14   Adding wind logging sections - Matt Little
  5/2/14   Sorting out sleep modes - Matt Little
  5/2/14   Adding HIH humidity/temperature sensor - Matt Little
  5/2/14   Sorting out Card Detect when card removed - Matt Little
  10/2/14  Sorting out card re-enter issue - Matt Little
  17/6/14  Adding pull-up enable for calibrate pin.- Matt Little
  17/6/14  Removing humidity sensor routines. Not needed -  Matt Little
  17/6/14  Adding debounce timer for anemometer pulses - Matt Little
  17/6/14  Adding Direction vane input - Matt Little
  10/7/14  Adding additional Anemometer input - Matt Little
  10/7/14  Needs additional interrupt pin added - Matt Little
  13/8/14  Added wind direction data - Matt Little
  15/8/14  Added 'I'm Alive' LED flash every 5 seconds - Matt Little
  
  TO DO
 
 // *********SD CARD DETAILS***************************	
 The SD card circuit:
 SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10
 ** Card detect - pin 6
 
 // ************ Real Time Clock code*******************
 A PCF8563 RTC is attached to pins:
 ** A4 - SDA (serial data)
 ** A5 - SDC (serial clock)
 ** D2 - Clock out - This gives a 1 second pulse to record the data
 
 RTC PCF8563 code details:
 By Joe Robertson, jmr
 orbitalair@bellsouth.net
 
**********************************************************************************************************/


/************ External Libraries*****************************/
//#include <OneWire.h>
//#include <DallasTemperature.h> 
#include <stdlib.h>
#include <Wire.h>          // Required for RTC
#include <Rtc_Pcf8563.h>   // RTC library
#include <avr/pgmspace.h>  // Library for putting data into program memory
#include <EEPROM.h>        // For writing values to the EEPROM
#include <SdFat.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <PinChangeInt.h>  // For additional interrupts

/* Local Application Libraries */
#include "utility.h"
#include "wind_direction.h"
#include "sd_interface.h"
#include "data_fields.h"

/************User variables and hardware allocation**********************************************/

#define COMMA ','

/******* SD CARD *************/
#define CHIP_SELECT_PIN 10 // The SD card Chip Select pin 10
#define CARD_DETECT_PIN 6  // The SD card detect is on pin 6

//File datafile;   // The logging file
String dataString;    // This is the holder for the data as a string. Start as blank
volatile long dataCounter = 0;  // This holds the number of seconds since the last data store


/*************Real Time Clock*******/
Rtc_Pcf8563 rtc;
#define I2C_RTC 0x51 // 7 bit address (without last bit - look at the datasheet)
#define RTC_INTERRUPT_VECTOR 0  // RTC interrupt - This is pin 2 of ardunio - which is INT0

/************* Pulse counter *******/
#define pulseInterrupt 1  // Pulse Counter Interrupt - This is pin 3 of arduino - which is INT1
#define ANEMOMETER1 3  //   This is digital pin the pulse is attached to
#define ANEMOMETER2 4  //   This is digital pin the pulse is attached to


/********* I/O Pins *************/
#define LEDred 5      // The output led is on pin 5
#define calibrate 7   // This controls if we are in serial calibrate mode or not
#define batteryPin A1  // For monitoring the battery voltage
#define directionPin A2  // For monitoring the wind direction

/********** Thermistor Data Storage ************/
#define thermistor A0  // This is the analog pin for the thermistor
float TempC = 0;  // This holds the converted value of temperature
char TempCStr[6];  // A string buffer to hold the converted string

// enumarating 3 major temperature scales
enum {
  T_KELVIN=0,
  T_CELSIUS,
  T_FAHRENHEIT
};
// Manufacturer data for episco k164 10k thermistor
// simply delete this if you don't need it
// or use this idea to define your own thermistors
//#define EPISCO_K164_10k 4300.0f,298.15f,10000.0f  // B,T0,R0
//#define GT_Thermistor_10k 4126.0f,298.15f,10000.0f  // B,T0,R0
#define Vishay_Thermistor_47k 4090.0f,298.15f,47000.0f  // B,T0,R0

///********* Thermistor Temperature sensor****************/
float temp;        // Temporary store for float

///********* Battery Voltage ****************/
float batteryVoltage;        // Temporary store for float
char BatteryVoltStr[5];      // Hold the battery voltage as a string

// ****** Serial Data Read***********
// Variables for the serial data read
char inByte;         // incoming serial char
String str_buffer = "";  // This is the holder for the string which we will display

//********Variables for the Filename*******************

static char deviceID[3]; // A buffer to hold the device ID

static long s_sampleTime = 2;  // This is the time between samples for the DAQ
                      // Sample time is stored in EEPROM in locations 2 & 3

static char s_currentDay = -1;

// Variables for the Pulse Counter
volatile long pulseCounter1 = 0;  // This counts pulses from the flow sensor  - Needs to be long to hold number
volatile long pulseCounter1Old = 0;  // This is storage for the old flow sensor - Needs to be long to hold number

volatile long pulseCounter2 = 0;  // This counts pulses from the flow sensor  - Needs to be long to hold number
volatile long pulseCounter2Old = 0;  // This is storage for the old flow sensor - Needs to be long to hold number

volatile boolean writedataflag = HIGH;  // A flag to tell the code when to write data

// Varibles for 'I'm alive' flash
volatile int aliveFlashCounter = 0;  // This is used to count to give flash every 10 seconds

boolean calibrateFlag = HIGH;  // This flag is lowered if we are in calibrate mode (switch ON)
static bool s_debugFlag = HIGH;    // Set this if you want to be in debugging mode.

//**********STRINGS TO USE****************************
String windDirection;

// These are Char Strings - they are stored in program memory to save space in data memory
// These are a mixutre of error messages and serial printed information
static const prog_char error[] PROGMEM = "ERROR ERROR ERROR";
static const prog_char reference[] PROGMEM = "The ref number is:";

/***************************************************
 *  Name:        createFileIfIsNewDay
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Creates a new CSV file if required (a new day)
 *
 ***************************************************/
static void createFileIfIsNewDay(void)
{
    char today = rtc.getDay();
    
    if(s_currentDay != s_currentDay)
    {
        // TODO: If the file cannot be created, what happens???
        (void)createCSVFileFromDate(today, rtc.getMonth(), rtc.getYear());
    }
}

/***************************************************
 *  Name:        pulse1
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Count pulses from Anemometer 1
 *
 ***************************************************/
void pulse1(void)
{
  // If the anemometer has spun around
  // Increment the pulse counter
  pulseCounter1++;
  // ***TO DO**** Might need to debounce this
}

/***************************************************
 *  Name:        pulse2
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Count pulses from Anemometer 2
 *
 ***************************************************/
void pulse2(void)
{
  // If the anemometer has spun around
  // Increment the pulse counter
  pulseCounter2++;
  // ***TO DO**** Might need to debounce this
}

/***************************************************
 *  Name:        onRTCInterrupt
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: I use the CLK_OUT from the RTC to give me exact 1Hz signal
 *               To do this I changed the initialise the RTC with the CLKOUT at 1Hz
 *
 ***************************************************/
void onRTCInterrupt()
{ 
  detachInterrupt(RTC_INTERRUPT_VECTOR);
  dataCounter++;
  aliveFlashCounter++;
  
  if(writedataflag==LOW&&dataCounter>=s_sampleTime)  // This stops us loosing data if a second is missed
  { 
    // If this interrupt has happened then we want to write data to SD card:
    // Save the pulsecounter value (this will be stored to write to SD card
    pulseCounter1Old = pulseCounter1;
    pulseCounter2Old = pulseCounter2;
   // Reset the pulse counter
    pulseCounter1 = 0;
    pulseCounter2 = 0;
    // Reset the DataCounter
    dataCounter = 0;  
    // Set the writedataflag HIGH
    writedataflag=HIGH;
  }
}

/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
{

  attachInterrupt(RTC_INTERRUPT_VECTOR, onRTCInterrupt, RISING);
  
  sleep_enable();
   
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
  
  byte old_ADCSRA = ADCSRA;  // Store the old value to re-enable 
  // disable ADC
  ADCSRA = 0;

  byte old_PRR = PRR;  // Store previous version on PRR
  // turn off various modules
  PRR = 0b11111111;
  
  sleep_cpu();
  /* The program will continue from here. */
  /************* ASLEEP *******************/
  
  // ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ //
  
  /************** WOKEN UP ***************/
  /* First thing to do is disable sleep. */
  sleep_disable();
  
  // turn ON various modules USART and ADC
  PRR = old_PRR;  
  
  // enable ADC
  ADCSRA = old_ADCSRA;  
}


/***************************************************
 *  Name:        setup
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Setup for the Arduino.           
 *
 ***************************************************/
void setup()
{
    Serial.begin(115200);
    Wire.begin();

    if (applicationIsInDebugMode())
    {
        Serial.println(F("*** DataLogger - Debug ON ***"));
    }
    //******Real Time Clock Set - up********
    // A4 and A5 are used as I2C interface.
    // D2 is connected to CLK OUT from RTC. This triggers an interrupt to take data
    // We need to enable pull up resistors
    pinMode(A4, INPUT);           // set pin to input
    digitalWrite(A4, HIGH);       // turn on pullup resistors
    pinMode(A5, INPUT);           // set pin to input
    digitalWrite(A5, HIGH);       // turn on pullup resistors
    pinMode(2,INPUT);    // Set D2 to be an input for the RTC CLK-OUT   

    //initialisetemp();  // Initialise the temperature sensors
    pinMode(LEDred,OUTPUT);    // Set D5 to be an output LED
    setSDPins(CARD_DETECT_PIN, CHIP_SELECT_PIN);

    //Set up digital data lines
    pinMode(calibrate,INPUT_PULLUP);

    // Battery voltage sensing
    pinMode(batteryPin,INPUT);

    //Set up direction input for wind vane
    pinMode(directionPin,INPUT); 

    // Put unused pins to INPUT to try and save power...   
    pinMode(9,INPUT);    
    pinMode(A3,INPUT); 

    setupRTC();  // Initialise the real time clock  

    initialiseSD();    // Initialise the SD card
    createCSVFileFromDate(rtc.getDay(), rtc.getMonth(), rtc.getYear());

    // Read the reference number from the EEROM
    deviceID[0] = char(EEPROM.read(0));
    deviceID[1] = char(EEPROM.read(1));

    // Read in the sample time from EEPROM
    char hiByte = EEPROM.read(2);
    char loByte = EEPROM.read(3);
    s_sampleTime = (hiByte << 8)+loByte;  // Get the sensor calibrate value 

    if (applicationIsInDebugMode())
    {
        Serial.print(F("Reference: "));
        Serial.print(deviceID[0]);
        Serial.println(deviceID[1]);
        Serial.print(F("Sample time: "));
        Serial.print(s_sampleTime);
        Serial.println(F(" seconds"));
    }

    analogReference(EXTERNAL);  // This should be default, but just to be sure

    // Interrupt for the 1Hz signal from the RTC
    attachInterrupt(RTC_INTERRUPT_VECTOR, onRTCInterrupt, RISING); 
    // Attach interrupts for the pulse counting
    pinMode(ANEMOMETER1, INPUT); 
    digitalWrite(ANEMOMETER1, HIGH);
    PCintPort::attachInterrupt(ANEMOMETER1, &pulse1, FALLING);  // add more attachInterrupt code as required
    pinMode(ANEMOMETER2, INPUT); 
    digitalWrite(ANEMOMETER2, HIGH);
    PCintPort::attachInterrupt(ANEMOMETER2, &pulse2, FALLING);  

}


/***************************************************
 *  Name:        main loop
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Main application loop.
 *
 ***************************************************/

void loop()
{
    // *********** WIND DIRECTION **************************************  
    // Want to measure the wind direction every second to give good direction analysis
    // This can be checked every second and an average used

    convertWindDirection(analogRead(directionPin));    // Run this every second. It increments the windDirectionArray 

    if(aliveFlashCounter>=10)
    {
        // Flash the LED every 10 seconds to show alive
        pinMode(LEDred,OUTPUT);    // Set LED to be an output LED 
        digitalWrite(LEDred, HIGH);   // set the LED ON
        delay(1);
        digitalWrite(LEDred, LOW);   // set the LED OFF 
        aliveFlashCounter=0;  // Reset the counter  
    }
  
    if(writedataflag==HIGH)
    {  
        pinMode(LEDred,OUTPUT);    // Set LED to be an output LED 
        digitalWrite(LEDred, HIGH);   // set the LED ON

        // *********** WIND SPEED ******************************************
        // Want to get the number of pulses and average into the sample time
        // This gives us the average wind speed
        // pulsecounterold holds the value of pulses.
        // This can be converted into the wind speed using the time and 
        // the pulse-wind speed characterisitic of the anemometer.
        // Do this as post processing - pulse count is most important.

        // *********** WIND DIRECTION **************************************
        // This can be checked every second and an average used
        windDirection = getMostFrequentWindDirectionString();
        resetWindDirection();

        // *********** TEMPERATURE *****************************************
        // Two versions of this - either with thermistor or I2C sensor (if connected)
        // Thermistor version
        // Get the temperature readings and store to variables
        TempC = Temperature(thermistor,T_CELSIUS,Vishay_Thermistor_47k,47000.0f);
        dtostrf(TempC,2,2,TempCStr);  // Convert the temperature value (double) into a string

        if(applicationIsInDebugMode())
        {
            Serial.print(F("Therm: "));
            Serial.println(TempCStr);  
        }   

        // *********** BATTERY VOLTAGE ***************************************
        // From Vcc-47k--47k-GND potential divider
        // This is to test in case battery voltage has dropped too low - alert?
        batteryVoltage = float(analogRead(batteryPin))*(3.3/1024.0)*2.0;        // Temporary store for float
        dtostrf(batteryVoltage,2,2,BatteryVoltStr);     // Hold the battery voltage as a string

        // ******** put this data into a file ********************************
        // ****** Check filename *********************************************
        // Each day we want to write a new file.
        // Compare date with previous stored date, every second
        createFileIfIsNewDay();

        // ********* Create string of data **************************
        String date = String(rtc.formatDate(RTCC_DATE_WORLD));  

        dataString =  String(deviceID[0]); 
        dataString += deviceID[1];  // Reference
        dataString += COMMA;
        dataString += date;  // Date
        dataString += COMMA;
        dataString += String(rtc.formatTime()); // Time
        dataString += COMMA;
        dataString += String(pulseCounter1Old); // Wind pulses 1
        dataString += COMMA;
        dataString += String(pulseCounter2Old); // Wind pulses 2
        dataString += COMMA;
        dataString += windDirection; // Wind direction
        dataString += COMMA;
        dataString += TempCStr;  // Temperature (Thermistor)
        dataString += COMMA;
        dataString += BatteryVoltStr;  // Battery voltage  

        // ************** Write it to the SD card *************
        // This depends upon the card detect.
        // If card is there then write to the file
        // If card has recently been inserted then initialise the card/filenames
        // If card is not there then flash LEDs

        if(sdCardHasBeenInserted())
        {
            delay(100);  // Wait for switch to settle down.
            // There was no card previously so re-initialise and re-check the filename
            initialiseSD();
            createCSVFileFromDate(rtc.getDay(), rtc.getMonth(), rtc.getYear());
        }

        writeToSD(&dataString);

        // Finish up write routine here:    
        digitalWrite(LEDred, LOW);   // set the LED OFF 
        pinMode(LEDred,INPUT);    // Set LED to be an INPUT - saves power   
        writedataflag=LOW;
    } 
  
    // Want to check the SD card every second
    if(sdCardIsPresent() == false)
    {
        pinMode(LEDred,OUTPUT);    // Set LED to be an output LED 
        // This meands there is no card present so flash the LED every second
        for(int x=0;x<=5;x++)
        {
            digitalWrite(LEDred, HIGH);   // set the LED ON
            delay(5);
            digitalWrite(LEDred, LOW);   // set the LED ON
            delay(50);     
        }
    } 

    if(applicationIsInDebugMode())
    {
        // DEBUGGING ONLY........
        Serial.print(F("Anemometer1: ")); Serial.flush();
        Serial.println(pulseCounter1, DEC); Serial.flush();
        Serial.print(F("Anemometer2: ")); Serial.flush();
        Serial.println(pulseCounter2, DEC); Serial.flush();
    }
  
    if(digitalRead(calibrate)==LOW)
    {    
        // We ARE in calibrate mode
        Serial.println(F("Calibrate"));
        getData();
        delay(500);  // Some time to read data

        pulseCounter1 = 0;
        pulseCounter2 = 0;
        dataCounter = 0;  
        writedataflag=LOW;
    }
    else
    {     
        attachInterrupt(RTC_INTERRUPT_VECTOR, onRTCInterrupt, RISING); 
        enterSleep();     
    }
}

// Set Up RTC routine
void setupRTC()
{
    // This section configures the RTC to have a 1Hz output.
    // Its a bit strange as first we read the data from the RTC
    // Then we load it back again but including the correct second flag
    rtc.getDateTime();
    rtc.formatDate(RTCC_DATE_WORLD);
    rtc.formatTime();

    uint8_t year_int = rtc.getYear();
    uint8_t day_int = rtc.getDay();
    uint8_t month_int = rtc.getMonth();  
    uint8_t hour_int = rtc.getHour();
    uint8_t min_int = rtc.getMinute();
    uint8_t sec_int = rtc.getSecond(); 

    if (applicationIsInDebugMode())
    {
        Serial.print(F("Date from RTC: "));
        Serial.print(day_int);
        Serial.print(F("-"));
        Serial.print(month_int);
        Serial.print(F("-"));
        Serial.print(year_int);
        Serial.print(F(" "));
        Serial.print(hour_int);
        Serial.print(F(":"));
        Serial.print(min_int);
        Serial.print(F(":"));
        Serial.println(sec_int);
    }

    Wire.begin(); // Initiate the Wire library and join the I2C bus as a master
    Wire.beginTransmission(I2C_RTC); // Select RTC
    Wire.write(0);        // Start address
    Wire.write(0);     // Control and status 1
    Wire.write(0);     // Control and status 2
    Wire.write(DecToBcd(sec_int));     // Second
    Wire.write(DecToBcd(min_int));    // Minute
    Wire.write(DecToBcd(hour_int));    // Hour
    Wire.write(DecToBcd(day_int));    // Day
    Wire.write(DecToBcd(2));    // Weekday
    Wire.write(DecToBcd(month_int));     // Month (with century bit = 0)
    Wire.write(DecToBcd(year_int));    // Year
    Wire.write(0b10000000);    // Minute alarm (and alarm disabled)
    Wire.write(0b10000000);    // Hour alarm (and alarm disabled)
    Wire.write(0b10000000);    // Day alarm (and alarm disabled)
    Wire.write(0b10000000);    // Weekday alarm (and alarm disabled)
    Wire.write(0b10000011);     // Output clock frequency enabled (1 Hz) ***THIS IS THE IMPORTANT LINE**
    Wire.write(0);     // Timer (countdown) disabled
    Wire.write(0);     // Timer value
    Wire.endTransmission();

     // The current day is how the application determines when to open a new file
    s_currentDay = day_int;
}

// Temperature function outputs float , the actual
// temperature
// Temperature function inputs
// 1.AnalogInputNumber - analog input to read from
// 2.OuputUnit - output in celsius, kelvin or fahrenheit
// 3.Thermistor B parameter - found in datasheet
// 4.Manufacturer T0 parameter - found in datasheet (kelvin)
// 5. Manufacturer R0 parameter - found in datasheet (ohms)
// 6. Your balance resistor resistance in ohms  

float Temperature(int AnalogInputNumber,int OutputUnit,float B,float T0,float R0,float R_Balance)
{
  float R,T,data;

  //R=1024.0f*R_Balance/float(analogRead(AnalogInputNumber))-R_Balance;
  
  // Changes as using thermistor to ground:
  data = float(analogRead(AnalogInputNumber));
  R=(data*R_Balance)/(1024.0f-data);
  
  T=1.0f/(1.0f/T0+(1.0f/B)*log(R/R0));

  switch(OutputUnit) {
    case T_CELSIUS :
      T-=273.15f;
    break;
    case T_FAHRENHEIT :
      T=9.0f*(T-273.15f)/5.0f+32.0f;
    break;
    default:
    break;
  };

  return T;
}


// **********************GET DATA SUBROUTINE*****************************************
// This sub-routine picks up and serial string sent to the device and sorts out a power string if there is one
// All values are global, hence nothing is sent/returned

void getData()
{
    // **********GET DATA*******************************************
    // We want to find the bit of interesting data in the serial data stream
    // If we write H then house number then the code will update the house number in EEPROM
    // **** aslo need to write code to update RTC

    for(int i = 0; i<10;i++)  // This helps us just take a 'chunk' of data so does not fill up serial buffer
    {
        // get incoming bytes:
        if (Serial.available() > 0) 
        {
            inByte = Serial.read(); 
            str_buffer+=inByte;

            if(inByte=='E')    // We read everything up to the byte 'E' which stands for END
            {
                processSerialInput();
            }
        }
    }
}

static void processSerialInput(void)
{

    char buffer[] = "00";
    int buffer_length = str_buffer.length();  // We also find the length of the string so we know how many char to display 
    // Depending upon what came before we update different values
    // To change the reference number we enter R00E, where 00 can be any number up to 99 

    for(int i = buffer_length; i>=0; i--)  // Check the buffer from the end of the data, working backwards
    {
        if(str_buffer[i]=='R')
        {
            // In this case we have changed the house number, so UPDATE and store in EEPROM
            deviceID[0]=str_buffer[i+1];
            deviceID[1]=str_buffer[i+2];
            Serial.print(getProgmemString(reference));
            Serial.print(deviceID[0]);
            Serial.println(deviceID[1]);
            EEPROM.write(0,deviceID[0]);
            EEPROM.write(1,deviceID[1]);
            initialiseSD();             
            createCSVFileFromDate(rtc.getDay(), rtc.getMonth(), rtc.getYear());
        }          
        if(str_buffer[i]=='T')
        {
            // In this case we have changed the TIME, so UPDATE and store to RTC
            // The time is in the format  HHMMSS

            buffer[0] = str_buffer[i+1]; buffer[1] = str_buffer[i+2];
            int hour = atoi(buffer);
            buffer[0] = str_buffer[i+3]; buffer[1] = str_buffer[i+4];
            int minute = atoi(buffer);
            buffer[0] = str_buffer[i+5]; buffer[1] = str_buffer[i+6];
            int second = atoi(buffer);
            //hr, min, sec into Real Time Clock
            rtc.setTime(hour, minute, second);

            initialiseSD();
            createCSVFileFromDate(rtc.getDay(), rtc.getMonth(), rtc.getYear());
            if (applicationIsInDebugMode())
            {
            Serial.print(F("Time set: "));
            Serial.println(String(rtc.formatTime())); // Time
            }
        }
        if(str_buffer[i]=='D')
        {
            // In this case we have changed the DATE, so UPDATE and store to RTC
            // The time is in the format  DDMMYY
            buffer[0] = str_buffer[i+1]; buffer[1] = str_buffer[i+2];
            int day = atoi(buffer);
            buffer[0] = str_buffer[i+3]; buffer[1] = str_buffer[i+4];
            int month = atoi(buffer);          
            buffer[0] = str_buffer[i+5]; buffer[1] = str_buffer[i+6];
            int year = atoi(buffer);          

            //day, weekday, month, century(1=1900, 0=2000), year(0-99)
            rtc.setDate(day, 3, month, 0, year);

            initialiseSD();
            createCSVFileFromDate(rtc.getDay(), rtc.getMonth(), rtc.getYear());
            if (applicationIsInDebugMode())
            {
                Serial.print(F("Date set: "));
                Serial.println(String(rtc.formatDate(RTCC_DATE_WORLD)));
            }
        }           
        if(str_buffer[i]=='S')
        {          
            // In this case we have changed the sample time, so UPDATE and store to EEPROM
            // Data will be in the form of 5 x chars, signifying XXXXX, a value from 00001 to 99999 seconds

            s_sampleTime = atol(&str_buffer[i+1]);  // Convert the string to a long int

            EEPROM.write(2, s_sampleTime >> 8);    // Do this seperately
            EEPROM.write(3, s_sampleTime & 0xff);
            if (applicationIsInDebugMode())
            {
                Serial.print(F("Sample Time:"));
                Serial.println(s_sampleTime);
            }
            dataCounter=0;  // Reset the data counter to start counting again.
        }
        if(str_buffer[i]=='?')
        {
            Serial.println("?");
        }
    }
    str_buffer="";  // Reset the buffer to be filled again
}

/*
 * Public Function Definitions
 */

bool applicationIsInDebugMode(void)
{
    return s_debugFlag;
}

