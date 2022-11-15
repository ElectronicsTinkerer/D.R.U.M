
#include "pico/stdlib.h"
#include <cstring>
#include <cstdlib>


#ifndef _PSEUDO_ARDUINO_H
#define _PSEUDO_ARDUINO_H

typedef uint8_t byte;
typedef bool boolean;

#define OUTPUT GPIO_OUT
#define INPUT GPIO_IN
#define INPUT_PULLUP 233 // Hopefully not used???
#define INPUT_PULLDOWN 234

#define micros() ((unsigned long)get_absolute_time())
#define delay(t) (sleep_ms(t))
#define delayMicroseconds(t) (sleep_us(t))
#define min(x, y) (((x) < (y)) ? (x) : (y))
// #define pinMode(p, t) (gpio_set_dir((p), (t)))
// #define digitalWrite(p, s) (gpio_put((p), (s)))
// #define digitalRead(p) (gpio_get((p))

inline long map(long val, long i_min, long i_max, long o_min, long o_max)
{
    return ((val - i_min) * (o_max - o_min)) / (i_max - i_min);
}

class Print
{
public:
    void pinMode(uint8_t, uint8_t);
    bool digitalRead(uint8_t);
    /* empty? */
};

#endif

