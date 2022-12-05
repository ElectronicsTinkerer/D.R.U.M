/**
 * DRUM firmware
 * 
 * Serial bus communication protocol struct typedef
 * 
 * Authors:
 * Zach Baldwin
 */

#ifndef DRUM_SERBUS_H
#define DRUM_SERBUS_H

typedef union ctlword {
    struct {
        unsigned int beat   : 4;
        signed int ubeat    : 4;
        unsigned int veloc  : 4;  // 0 = no hit
        unsigned int xresv  : 16; // RESERVED
        unsigned int modsel : 1;  // 0 = Module unselected, 1 = Module selected
        unsigned int fdesel : 1;  // 0 = Don't affect module select status,
                                  // 1 = Force unselection of module (Overrides modsel)
        unsigned int rwb    : 1;  // 0 = Write operation to module
                                  // 1 = Read operation from module
        unsigned int cs     : 1;  // "Chip Select":
                                  // 0 = module should ignore command
                                  // 1 = module should execute command
    } data;
    uint32_t raw;
} ctlword_t;

void intermodule_serbus_txrx(
    const PIO,
    const uint,
    const PIO,
    const uint,
    uint32_t *,
    uint32_t *,
    size_t,
    uint,
    uint
    );

#endif

