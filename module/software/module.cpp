/**
 * MODULE CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"

#include "module.pio.h"
#include "serbus.h"
#include "module.h"

volatile unsigned int is_selected;

unsigned int variability;

struct {
    uint sm;
    PIO pio;
    uint offs;
} intermodule_pio;

int main ()
{
    // Hardware init
    stdio_init_all();

    // Output Pins
    gpio_init(GPIO_DAC_LDAC);
    gpio_init(GPIO_DAC_CSB);
    gpio_init(GPIO_DAC_SCK);
    gpio_init(GPIO_DAC_SDAT);
    gpio_set_dir(GPIO_DAC_LDAC, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_CSB, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_SCK, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_SDAT, GPIO_OUT);

    // Input pins
    gpio_init(GPIO_SEL_BTN);
    gpio_set_dir(GPIO_SEL_BTN, GPIO_IN);

    // The "Hey, I've changed status" line
    gpio_init(MOD_STAT_IRQ);
    gpio_put(MOD_STAT_IRQ, 0);
    gpio_set_dir(GPIO_IN);
    
    // Init
    is_selected = False;
    variability = 0;
    clear_beats();

    // Init the PIO state machine which handles
    // intermodule communication


    // Set up all the interrupt handlers for the GPIOs
    gpio_set_irq_enabled_with_callback(
        GPIO_SEL_BTN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true, // (enable isr)
        &isr_gpio_handler
        );
    

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
// void isr_serbus(void)
// {
//     
// }


/**
 * Called whenever a GPIO-based interrupt occurs. This decides
 * which service routine to use based on the GPIO number.
 * 
 * @param gpio The source GPIO of the interrupt
 * @param event
 */
void isr_gpio_handler(unsigned int gpio, uint32_t event)
{
    switch (gpio) {
    case GPIO_SEL_BTN:
        isr_mod_sel_btn(gpio, event);
        break;
        
    default:
        break;
    }
}


/**
 * Handles generation of an output pulse on the MOD_STAT_IRQ line
 * whenever the state of the select button changes
 * 
 * @param gpio 
 * @param event 
 */
void isr_mod_sel_btn(unsigned int gpio, uint32_t event)
{
    is_selected = gpio_get(gpio);

    // Toggle IRQ line low to signal a change to the sequencer
    set_mod_stat(0);

    // Callback in 100 us to toggle irq line back high
    add_alarm_in_us(100, alarm_mod_stat_callback, NULL, false);
}


/**
 * Releases the module status line
 * 
 * @param id 
 * @param *user_data 
 * @return Time in us to fire again. 0 if disabled (see Pico SDK docs)
 */
uint64_t alarm_mod_stat_callback(alarm_id_t id, void *user_data)
{
    set_mod_stat(true); // Set back HIGH
    return 0; // Time in us to fire again in the future. 0 disables this
}


/**
 * Set the state of the module status line.
 * 
 * @param state true = release line to go high
 *              false = pull line low
 * @return 
 */
void set_mod_stat(bool state)
{
    if (state) {
        gpio_set_dir(GPIO_IN);
    } else {
        gpio_set_dir(GPIO_OUT);
    }
}


