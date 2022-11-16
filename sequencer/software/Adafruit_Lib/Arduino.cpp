
#include "Arduino.h"

// bool Print::digitalRead(uint8_t pin) {
    // return gpio_get(pin);
// }


char *itoa(int value, char *string, int radix)
{
    if (radix == 10) {
        sprintf(string, "%d", value);
    } else if (radix == 8) {
        sprintf(string, "%o", value);
    } else if (radix == 2) {
        sprintf(string, "%b", value);
    } else if (radix == 16) {
        sprintf(string, "%x", value);
    }
    return string; // ???
}

char *ltoa(long value, char *string, int radix)
{
    if (radix == 10) {
        sprintf(string, "%ld", value);
    } else if (radix == 8) {
        sprintf(string, "%lo", value);
    } else if (radix == 2) {
        sprintf(string, "%lb", value);
    } else if (radix == 16) {
        sprintf(string, "%lx", value);
    }
    return string; // ???
}

char *utoa(unsigned value, char *string, int radix)
{
    if (radix == 10) {
        sprintf(string, "%u", value);
    } else if (radix == 8) {
        sprintf(string, "%o", value);
    } else if (radix == 2) {
        sprintf(string, "%b", value);
    } else if (radix == 16) {
        sprintf(string, "%x", value);
    }
    return string; // ???
}

char *ultoa(unsigned long value, char *string, int radix)
{
    if (radix == 10) {
        sprintf(string, "%lu", value);
    } else if (radix == 8) {
        sprintf(string, "%o", value);
    } else if (radix == 2) {
        sprintf(string, "%b", value);
    } else if (radix == 16) {
        sprintf(string, "%x", value);
    }
    return string; // ???

}

char *dtostrf(double val, signed char width, unsigned char prec, char *sout)
{
    // https://linuxhint.com/arduino-dtostrf-function/
    // https://github.com/arduino/ArduinoCore-API/blob/master/api/deprecated-avr-comp/avr/dtostrf.h
    // THIS FUNCTION IS NOT PROPERLY IMPLEMENTED!
    sprintf(sout, "%f", val);
    return sout; // ???
}
