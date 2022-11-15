
#include "Arduino.h"

bool Print::digitalRead(uint8_t pin) {
    return gpio_get(pin);
}
