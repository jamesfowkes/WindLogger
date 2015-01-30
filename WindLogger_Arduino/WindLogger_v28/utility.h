#ifndef _UTILITY_H_
#define _UTILITY_H_

template <typename T>
T getMaximum(T * array, uint16_t length);

template <typename T>
uint16_t getIndexOfMaximum(T * array, uint16_t length);

template <typename T>
void fillArray(T * array, T value, uint16_t size);

char* getProgmemString(const char* str);

char DecToBcd(char value);

#define BETWEEN_INC(x, a, b) ((x >= a) && (x <= b))
#define BETWEEN_EXC(x, a, b) ((x > a) && (x < b))

#define BETWEEN_EXC_HI(x, a, b) ((x >= a) && (x < b))
#define BETWEEN_EXC_LO(x, a, b) ((x > a) && (x <= b))

#endif
