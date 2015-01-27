/*
 * wind_direction.c
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
 
#include <arduino.h>
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

static char * s_directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    
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
    
    if(reading>0&&reading<100)
    {
        eDirection = DIR_W;
    }
    else if(reading>100&&reading<200)
    {
        eDirection = DIR_NW;
    }
    else if(reading>200&&reading<350)
    {
        eDirection = DIR_N;
    }
    else if(reading>350&&reading<450)
    {
        eDirection = DIR_SW;
    }  
    else if(reading>450&&reading<650)
    {
        eDirection = DIR_NE;
    }  
    else if(reading>650&&reading<800)
    {
        eDirection = DIR_S;
    }
    else if(reading>800&&reading<900)
    {
        eDirection = DIR_SE;
    }
    else if(reading>900&&reading<1024)
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
char * getMostFrequentWindDirectionString(void)
{
    CARDINAL_DIRECTION mostFrequentDirection = getMostFrequentWindDirection();
    return cardinalDirectionToString( mostFrequentDirection );
}

/******** cardinalDirectionToString *********
 * Maps CARDINAL_DIRECTION enumeration to relevant string
 */
char * cardinalDirectionToString(CARDINAL_DIRECTION eDirection)
{
    if (eDirection < DIR_INVALID)
    {
        return s_directions[eDirection];
    }
    else
    {
        return "";
    }
}

void resetWindDirection(void)
{
    fillArray(s_windDirectionCount, (uint16_t)8, 0);
}