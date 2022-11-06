/**
 * MODULE CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#include "serbus.h"
#include "module.h"

volatile unsigned int is_selected;

unsigned int variability;
    

int main ()
{
    // Init
    is_selected = False;
    variability = 0;
    clear_beats();


    // Event loop
    while (1) {
        variability = read_variability_pot();

        // TODO: handle button press
    }
}


/**
 * Reset the sequencer grid
 */
void clear_beats(void)
{

}


/**
 * Read the analog value of the variability pot and return a numberically-
 * binned value corresponding to it.
 */
unsigned int read_variability_pot(void)
{

}


/**
 * Handle serial communication with sequencer.
 * Called when the CBUS_DRDY line is strobed
 */
void isr_serbus(void)
{
    
}



