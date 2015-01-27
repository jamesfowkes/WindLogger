#ifndef _UTILITY__H_
#define _UTILITY__H_

template <typename T>
T getMaximum(T * array, uint16_t length);

template <typename T>
uint16_t getIndexOfMaximum(T * array, uint16_t length);

template <typename T>
void fillArray(T * array, T value, uint16_t size);

#endif
