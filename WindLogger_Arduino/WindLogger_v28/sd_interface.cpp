/*
 * sd.cpp
 * 
 * Routines to read/write an SD card
 *
 * Author: James Fowkes
 * Adapted from original software by Matt Little
 *
 * www.re-innovation.co.uk
 */

/*
 * Arduino/C++ Library Includes
 */

#include <Arduino.h>
#include <SdFat.h>

/*
 * Local Application Includes
 */

#include "application.h"
#include "sd_interface.h"
#include "utility.h"
#include "filename.h"
#include "data_fields.h"

/*
 * Private Variables
 */

static int s_cardDetectPin;
static int s_chipSelectPin;

// Assume a card is inserted
static bool s_lastCardDetect = true;
static bool s_cardDetect = true;

static SdFat s_sd; // SD file system object
static SdFile s_datafile; 

static const prog_char s_success[] PROGMEM = "success.";
static const prog_char s_initialiseSD[] PROGMEM = "SD card Initialised";
static const prog_char s_errorOpen[] PROGMEM = "Error opening file";
static const prog_char s_fileExists[] PROGMEM = "File already exists";
static const prog_char s_headersOK[] PROGMEM = "Written headers to file";

/******** close *********
 * Close the currently open file
 */
static void close(void)
{
    s_datafile.sync();
    s_datafile.close();
}

static void updateCardDetect(void)
{
    // Save the last card detect state
    s_lastCardDetect = s_cardDetect;
    // Card is present if card detect is LOW
    s_cardDetect = digitalRead(s_cardDetectPin) == LOW;
}

/******** setSDPins *********/
void setSDPins(int cardDetectPin, int chipSelectPin)
{
    s_cardDetectPin = cardDetectPin;
    s_chipSelectPin = chipSelectPin;
    pinMode(cardDetectPin, INPUT);
    pinMode(chipSelectPin, OUTPUT);
}

/******** initialiseSD *********/
bool initialiseSD(void)
{
    if (!sdCardIsPresent()) { return false; }
    // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors 
    // We use SPI_HALF_SPEED here as I am using resistor level shifters.
    //if (!sd.begin(chipSelect, SPI_HALF_SPEED)) sd.initErrorHalt();

    // make sure that the default chip select pin is set to
    // output, even if you don't use it:

    bool success = false;
    pinMode(s_chipSelectPin, OUTPUT);

    // see if the card is present and can be initialized:
    success = s_sd.begin(s_chipSelectPin, SPI_HALF_SPEED);

    if (applicationIsInDebugMode())
    {
        Serial.print(F("Initalising SD card... "));
        if (success)
        {
            Serial.println(getProgmemString(s_success));
        }
        else
        {
            Serial.println(F("Failed"));
        }
    }

    return success;
}

bool createCSVFileFromDate(int day, int month, int year)
{
    bool success = false;

    if (!sdCardIsPresent()) { return false; }

    if (s_datafile.isOpen())
    {
        close(); // Ensure current file is closed before a new one is created
    }

    setFilenameFromDate(day, month, year);
    char const * const pCurrentFilename = getFilename();    

    if(applicationIsInDebugMode())
    {
        Serial.print(F("Creating file: "));
        Serial.print(pCurrentFilename);
        Serial.print(F(": "));
        Serial.flush();
    }

    if(!s_sd.exists(pCurrentFilename))
    {
        // open the file for write at end like the Native SD library
        success = s_datafile.open(pCurrentFilename, O_RDWR | O_CREAT | O_AT_END);
        
        if (applicationIsInDebugMode())
        {
            if (success)
            {
                Serial.println(getProgmemString(s_success)); Serial.flush();
            }
            else
            {
                Serial.println(getProgmemString(s_errorOpen)); Serial.flush();
            }
        }

        if (success)
        {
            // Write the file header
            char * pHeader = getFileHeaderString();
            bool success = writeToSD(pHeader);

            if(success && applicationIsInDebugMode())
            {
                Serial.println(getProgmemString(s_headersOK)); Serial.flush();
            }
        }
    }
    else if(applicationIsInDebugMode())
    {
        Serial.println(getProgmemString(s_fileExists)); Serial.flush();
    }

    return success;
}

/******** writetoSD ********* 
 * This routine write specified data string to the current file on the card
 */

bool writeToSD(char * pDataChars)
{
    // Alias for writeToSD(String)
    String dataString = String(pDataChars);
    return writeToSD(&dataString);
}

bool writeToSD(String *dataString)
{
    if (!sdCardIsPresent()) { return false; }

    bool success = false;

    char const * const pCurrentFilename = getFilename();
    if(applicationIsInDebugMode())
    {
        Serial.print(F("SD card write to "));
        Serial.print(pCurrentFilename);
        Serial.print(F("..."));
    }

    // if the file is available, write to it:
    if (s_sd.exists(pCurrentFilename))
    {
        success = s_datafile.isOpen();

        if (!success)
        {
            success = s_datafile.open(pCurrentFilename, O_RDWR | O_CREAT | O_AT_END);
        }

        if(success)
        {
            s_datafile.println(*dataString);
            s_datafile.sync();

            if(applicationIsInDebugMode()) { Serial.println(); }
            Serial.println(*dataString); // Always print the CSV line
            Serial.flush();
            
        }
    }  

    if(applicationIsInDebugMode())
    {
        if (success)
        {
            Serial.println(getProgmemString(s_success));
        }
        else
        {
            Serial.println(getProgmemString(s_errorOpen));
        }        
    }

    return success;
}

bool sdCardIsPresent(void)
{
    updateCardDetect();

    if((s_cardDetect == false) && applicationIsInDebugMode())
    {
        Serial.println(F("No SD card present")); Serial.flush();
    }

    return (s_lastCardDetect == true) && (s_cardDetect == true);
}

bool sdCardHasBeenInserted(void)
{
    updateCardDetect();
    bool inserted = (s_lastCardDetect == false) && (s_cardDetect == true);

    if(inserted && applicationIsInDebugMode())
    {
        Serial.println(F("SD card inserted")); Serial.flush();
    }
    return inserted;
}

