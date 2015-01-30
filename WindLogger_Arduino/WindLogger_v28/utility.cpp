/*
 * utility.cpp
 * 
 * A collection of generic utility routines
 *
 * Author: James Fowkes
 * Adapted from original software by Matt Little
 *
 * www.re-innovation.co.uk
 */

/* Arduino/AVR Includes */
#include <avr/pgmspace.h>  // Library for putting data into program memory

/*
 * Standard library includes
 */
#include <stdint.h>
#include "utility.h"

/* getMaximum, getIndexOfMaximum
 * Template functions for getting the maximum of an array
 */

template <typename T>
T getMaximum(T * array, uint16_t length)
{
    return array[ getIndexOfMaximum(array, length) ];
}

template <typename T>
uint16_t getIndexOfMaximum(T * array, uint16_t length)
{
    uint16_t index = 1;
    uint16_t max_index = 0;
    
    while (index < length)
    {
        if (array[index] > array[max_index]) { max_index = index; }
        index++;
    }
    
    return max_index;
}

template <typename T>
void fillArray(T * array, T value, uint16_t size)
{
    uint16_t index = 0;
    
    if (array)
    {
        for(index = 0; index < size; index++)
        {
            array[index] = value;
        }
    }
}

// Get a string from program memory
#define MAX_STRING 80      // Sets the maximum length of string probably could be lower
static char s_stringBuffer[MAX_STRING];  // A buffer to hold the string when pulled from program memory
char* getProgmemString(const char* str) {
	strcpy_P(s_stringBuffer, (char*)str);
	return s_stringBuffer;
}

// Converts a decimal to BCD (binary coded decimal)
char DecToBcd(char value){
  return (value / 10 * 16 + value % 10);
}

// The getMaximum, getIndexOfMaximum templates must be instantiated for all expected values
template uint8_t getMaximum(uint8_t *, uint16_t);
template int8_t getMaximum(int8_t *, uint16_t);
template uint16_t getMaximum(uint16_t *, uint16_t);
template int16_t getMaximum(int16_t *, uint16_t);
template uint32_t getMaximum(uint32_t *, uint16_t);
template int32_t getMaximum(int32_t *, uint16_t);
template float getMaximum(float *, uint16_t);

template uint16_t getIndexOfMaximum(uint8_t *, uint16_t);
template uint16_t getIndexOfMaximum(int8_t *, uint16_t);
template uint16_t getIndexOfMaximum(uint16_t *, uint16_t);
template uint16_t getIndexOfMaximum(int16_t *, uint16_t);
template uint16_t getIndexOfMaximum(uint32_t *, uint16_t);
template uint16_t getIndexOfMaximum(int32_t *, uint16_t);
template uint16_t getIndexOfMaximum(float *, uint16_t);

template void fillArray(uint8_t * array, uint8_t value, uint16_t size);
template void fillArray(int8_t * array, int8_t value, uint16_t size);
template void fillArray(uint16_t * array, uint16_t value, uint16_t size);
template void fillArray(int16_t * array, int16_t value, uint16_t size);
template void fillArray(uint32_t * array, uint32_t value, uint16_t size);
template void fillArray(int32_t * array, int32_t value, uint16_t size);
template void fillArray(float * array, float value, uint16_t size);
