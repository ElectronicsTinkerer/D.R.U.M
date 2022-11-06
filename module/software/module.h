/**
 * MODULE CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */


#ifndef DRUM_MODULE_H
#define DRUM_MODULE_H

// Useful things
#define True 1
#define False 0

void clear_beats(void);
unsigned int read_variability_pot(void);
void isr_serbus(void);

#endif

