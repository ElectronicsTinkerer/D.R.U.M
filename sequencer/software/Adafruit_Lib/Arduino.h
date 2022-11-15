
#ifndef _PSEUDO_ARDUINO_H
#define _PSEUDO_ARDUINO_H

typedef uint8_t byte;
typedef uint8_t boolean;

#define micros() ((unsigned long)get_absolute_time())

class Print
{
    /* empty? */
};

#endif

