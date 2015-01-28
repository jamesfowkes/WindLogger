#ifndef _WIND_DIRECTION_H_
#define _WIND_DIRECTION_H_

enum cardinal_direction
{
    DIR_N,
    DIR_NE,
    DIR_E,
    DIR_SE,
    DIR_S,
    DIR_SW,
    DIR_W,
    DIR_NW,
    DIR_INVALID
};
typedef enum cardinal_direction CARDINAL_DIRECTION;

void resetWindDirection(void);
bool convertWindDirection(int reading);
CARDINAL_DIRECTION getMostFrequentWindDirection(void);
const char *  getMostFrequentWindDirectionString(void);
const char *  cardinalDirectionToString(CARDINAL_DIRECTION eDirection);

#endif
