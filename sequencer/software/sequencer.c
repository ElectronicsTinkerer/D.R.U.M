/**
 * SEQUENCER CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#include "hardware/gpio.h"

#include "serbus.h"
#include "sequencer.h"

// All of these can be modified by the ISRs
volatile unsigned int is_running;
volatile unsigned int is_mod_selected;
volatile unsigned int bpm;
volatile unsigned int time_sig;
volatile unsigned int current_beat;
repeating_timer_t timer;

int main ()
{
    // Hardware init
    stdio_init_all();

    // Initialize Sequencer states
    is_running = false;
    is_mod_selected = false;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    current_beat = 0;

    // Register ISRs to GPIO and Timer events
    // Tempo Encoder
    gpio_set_irq_enabled_with_callback(
        TEMPO_ENC0,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_tempo_encoder
        );
    gpio_set_irq_enabled_with_callback(
        TEMPO_ENC1,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_tempo_encoder
        );

    // Time signature encoder
    gpio_set_irq_enabled_with_callback(
        GPIO_ENC0,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_time_sig_encoder
        );
    gpio_set_irq_enabled_with_callback(
        TSIG_ENC1,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_time_sig_encoder
        );

    // Play / pause button
    gpio_set_irq_enabled_with_callback(
        PLYSTP_BTN,
        GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_play_pause
        );

    // Module selection status update
    gpio_set_irq_enabled_with_callback(
        MOD_STAT_IRQ,
        GPIO_IRQ_EDGE_RISE,
        true, // Enable interrupt
        &isr_module_status
        );

    // Set up the tick timer for micro beat generation    

    // Negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us((-1000000 * 60) / bpm, timer_callback, NULL, &isr_timer)) {
        // printf("Failed to add timer\n");
        // ERROR!
        return 1;
    }



    

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
    // TODO
}


/**
 * Update the colors of the button matrix based on the current sequencer
 * state.
 */
void update_buttons(void)
{
    // TODO
}


/**
 * Handles generation of synchronization "tick" pulses with all modules
 */
bool isr_timer(repeating_timer_t *rt);
{
    // TODO!

    return true; // keep repeating    
}


/**
 * Handles detection of module status changes via the CBUS_EXTRA 
 * interrupt line
 * 
 * @param The GPIO pin number
 * @param The GPIO event type */
void isr_module_status(unsigned int gpio, uint32_t event)
{
    // TODO
}


/**
 * Handles updating of the global tempo value when the tempo encoder
 * is rotated
 * 
 * @param The GPIO pin number
 * @param The GPIO event type
 */
void isr_tempo_encoder(unsigned int gpio, uint32_t event)
{
    // TODO
}


/**
 * Handles the toggling of the is_running flag based on the play/pause
 * button input
 * 
 * @param The GPIO pin number
 * @param The GPIO event type
 */
void isr_play_pause(unsigned int gpio, uint32_t event)
{
    is_running = !is_running;
}


/**
 * Handles cycling of the time signature when the time signature 
 * encoder is rotated
 * 
 * @param The GPIO pin number
 * @param The GPIO event type
 */
void isr_time_sig_encoder(unsigned int gpio, uint32_t event)
{

}


// TODO: This might not be possible, depending on whether or not the pad can s
/**
 * Handle a button press on the squish pad
 * 
 * @param The GPIO pin number
 * @param The GPIO event type
 */
void isr_pad_event(void)
{
    // TODO
}



