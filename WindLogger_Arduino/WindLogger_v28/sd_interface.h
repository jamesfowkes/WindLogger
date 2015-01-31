#ifndef _SD_H_
#define _SD_H_

/* Initialisation functionality */
void setSDPins(int cardDetectPin, int chipSelectPin);
bool initialiseSD(void);

/* File access functionality */
bool createCSVFileFromDate(int day, int month, int year);

/* Read/write functionality */
bool writeToSD(String *dataString);
bool writeToSD(char * pDataChars);

/* Card Detection functionality */
bool sdCardIsPresent(void);
bool sdCardHasBeenInserted(void);

#endif

