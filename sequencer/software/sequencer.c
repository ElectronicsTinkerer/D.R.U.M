/**
 * SEQUENCER CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#include "serbus.h"
#include "sequencer.h"

// All of these can be modified by the ISRs
volatile unsigned int is_running;
volatile unsigned int is_mod_selected;
volatile unsigned int bpm;
volatile unsigned int time_sig;
volatile unsigned int current_beat;

int main ()
{
    // Initialize everything
    is_running = False;
    is_mod_selected = False;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    current_beat = 0;

    // TODO: Register ISRs to GPIO and Timer events
    

    // Main event loop
    while (1) {
        update_screen();
        update_buttons();
    }
}


/**
 * Updates the screen. Displays the current tempo, time signature,
 * Play/Pause status, and relevant information during module programming
 */
void update_screen(void)
{
    
}


/**
 * Update the colors of the button matrix based on the current sequencer
 * state.
 */
void update_buttons(void)
{

}


/**
 * Handles generation of synchronization "tick" pulses with all modules
 */
void isr_timer(void)
{

}


/**
 * Handles detection of module status changes via the CBUS_EXTRA 
 * interrupt line
 */
void isr_module_status(void)
{

}


/**
 * Handles updating of the global tempo value when the tempo encoder
 * is rotated
 */
void isr_tempo_encoder(void)
{

}


/**
 * Handles the toggling of the is_running flag based on the play/pause
 * button input
 */
void isr_play_pause(void)
{

}


/**
 * Handles cycling of the time signature when the time signature 
 * encoder is rotated
 */
void isr_time_sig_encoder(void)
{

}


// TODO: This might not be possible, depending on whether or not the pad can s
/**
 * Handle a button press on the squish pad
 */
void isr_pad_event(void)
{

}



