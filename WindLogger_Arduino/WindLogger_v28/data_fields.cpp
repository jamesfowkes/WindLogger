/*
 * data_fields.cpp
 * 
 * This module handles storage of the data fields and headers
 *
 * Author: James Fowkes
 * Adapted from original software by Matt Little
 *
 * www.re-innovation.co.uk
 */

/*
 * Local Application Includes
 */

#include "data_fields.h"

/*
 * Public Function Definitions
 */

static char s_fieldnameString[MAX_FIELD_COUNT * MAX_FIELDNAME_LENGTH] =
    "Reference, Date, Time, Wind1, Wind2, Direction, Thermistor, Battery Voltage";

void setFieldFromConfigString(void)
{

}

char * getFileHeaderString(void)
{
   return s_fieldnameString;
}
