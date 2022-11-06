/**
 * SEQUENCER CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#ifndef DRUM_SEQUENCER_H
#define DRUM_SEQUENCER_H

// Useful things
#define True 1
#define False 0

// Tempo settings
#define BPM_MIN 20
#define BPM_DEFAULT 120
#define BPM_MAX 600

// Time signature settings
#define TS_DEFAULT 1

unsigned int time_signatures[][2] = {
    {3, 4},
    {4, 4},
    {7, 8}
};


// Function signatures
void update_screen(void);
void update_buttons(void);
void isr_timer(void);
void isr_module_status(void);
void isr_tempo_encoder(void);
void isr_play_pause(void);
void isr_time_sig_encoder(void);
void isr_pad_event(void); // This might not be possible, depending on whether or not the pad can send interrupts

#endif

