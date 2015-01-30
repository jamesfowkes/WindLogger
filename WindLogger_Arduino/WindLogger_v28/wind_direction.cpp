/*
 * wind_direction.cpp
 * 
 * Routines for handling conversion/formatting etc. of wind direction values
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
#include <limits.h>

/*
 * Local Application Includes
 */

#include "wind_direction.h"
#include "utility.h"

/*
 * Private Variables
 */
 
static uint16_t s_windDirectionCount[] = {0,0,0,0,0,0,0,0};  //Holds the frequency of the wind direction

/* To store direction strings in program memory, declare each string individually then store array */
static const char s_n[] PROGMEM = "N";
static const char s_ne[] PROGMEM = "NE";
static const char s_e[] PROGMEM = "E";
static const char s_se[] PROGMEM = "SE";
static const char s_s[] PROGMEM = "S";
static const char s_sw[] PROGMEM = "SW";
static const char s_w[] PROGMEM = "W";
static const char s_nw[] PROGMEM = "NW";

static const char * s_directions[] = {s_n, s_ne, s_e, s_se, s_s, s_sw, s_w, s_nw};
    
/* 
 * Public Functions
 */

/******** convertWindDirection *********
 * This routine takes in an analog read value and converts it into a wind direction
 * The Wind vane uses a series of resistors to show what direction the wind comes from
 * The different values are:
 *    R1 = 33k  => 238 N
 *    R2 = 8.2k => 562 NE
 *    R3 = 1k => 930 E
 *    R4 = 2.2k => 839 SE
 *    R5 = 3.9k => 736 S
 *    R6 = 16k => 394 SW
 *    R7 = 120k => 79 W
 *    R8 = 64.9k => 137 NW
 * 
 * Assuming a 10K resistor in series with the selected resistor
 * This means we can 'band' the data into 8 bands
 * 
 * The function will increment the value in the s_windDirectionCount array.
 * The array contains * values corresponding to N, NE etc. as per the CARDINAL_DIRECTION enum.
 * If the array value is already as its maximum (65535 on most Arduino platforms), no action is taken.
 *
 * Arguments:
 * reading - the reading to convert (0 to 1023)
 *   
 * Returns:
 *   True/false value indicating successful conversion
 */

bool convertWindDirection(int reading)
{
    // The reading has come from the ADC
    
    CARDINAL_DIRECTION eDirection = DIR_INVALID;
    
    if(BETWEEN_INC(reading, 0, 99))
    {
        eDirection = DIR_W;
    }
    else if(BETWEEN_INC(reading, 100, 199))
    {
        eDirection = DIR_NW;
    }
    else if(BETWEEN_INC(reading, 200, 349))
    {
        eDirection = DIR_N;
    }
    else if(BETWEEN_INC(reading, 350, 449))
    {
        eDirection = DIR_SW;
    }  
    else if(BETWEEN_INC(reading, 450, 649))
    {
        eDirection = DIR_NE;
    }  
    else if(BETWEEN_INC(reading, 650, 799))
    {
        eDirection = DIR_S;
    }
    else if(BETWEEN_INC(reading, 800, 899))
    {
        eDirection = DIR_SE;
    }
    else if(BETWEEN_INC(reading, 900, 1023))
    {
        eDirection = DIR_E;
    }
    else
    {
        // This is an error reading
    }
    
    // Test for valid direction and no rollover before incrementing
    if (eDirection != DIR_INVALID)
    {
        if (s_windDirectionCount[eDirection] < USHRT_MAX)
        {
            s_windDirectionCount[eDirection]++;
        }
    }
    
    return eDirection != DIR_INVALID;
}

/******** getMostFrequentWindDirection *********
 * Returns the most frequent wind direction in the array as a cardinal direction
 */
CARDINAL_DIRECTION getMostFrequentWindDirection(void)
{
    return (CARDINAL_DIRECTION)getIndexOfMaximum(s_windDirectionCount, 8);
}

/******** getMostFrequentWindDirectionString *********
 * Returns the most frequent wind direction in the array as a string
 */
const char * getMostFrequentWindDirectionString(void)
{
    CARDINAL_DIRECTION mostFrequentDirection = getMostFrequentWindDirection();
    return cardinalDirectionToString( mostFrequentDirection );
}

/******** cardinalDirectionToString *********
 * Maps CARDINAL_DIRECTION enumeration to relevant string
 */
const char * cardinalDirectionToString(CARDINAL_DIRECTION eDirection)
{
    if (eDirection < DIR_INVALID)
    {
        return getProgmemString(s_directions[eDirection]);
    }
    else
    {
        return "";
    }
}

void resetWindDirection(void)
{
    fillArray(s_windDirectionCount, (uint16_t)0, 8);
}
