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

volatile bool is_selected;

unsigned int variability;

volatile beat_data_t beats[16];

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
    gpio_init(GPIO_DAC_LDACB);
    gpio_init(GPIO_DAC_CSB);
    gpio_init(GPIO_DAC_SCK);
    gpio_init(GPIO_DAC_SDAT);
    gpio_set_dir(GPIO_DAC_LDACB, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_CSB, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_SCK, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_SDAT, GPIO_OUT);

    // Input pins
    gpio_init(GPIO_SEL_BTN);
    gpio_set_dir(GPIO_SEL_BTN, GPIO_IN);

    // The "Hey, I've changed status" line
    gpio_init(MOD_STAT_IRQ);
    gpio_put(MOD_STAT_IRQ, 0);
    gpio_set_dir(MOD_STAT_IRQ, GPIO_IN);
    
    // Init
    is_selected = false;
    variability = 0;
    clear_beats();

    // Init the PIO state machine which handles
    // intermodule communication
    // NOTE: If the PIO number is changed for this,
    // make sure to update the number for the IRQ handler
    // in the code that follows.
    intermodule_pio.pio = pio0;
    intermodule_pio.sm = pio_claim_unused_sm(intermodule_pio.pio, true);
    intermodule_pio.offs = pio_add_program(intermodule_pio.pio, &module_sio_program);
    module_sio_init(
        intermodule_pio.pio,
        intermodule_pio.sm,
        intermodule_pio.offs,
        GPIO_CBUS_SCK,
        GPIO_CBUS_DRDY,
        GPIO_CBUS_SDOUT,
        GPIO_CBUS_SDIN
        );

    // The PIO for the module uses an interrupt to tell
    // the main program that there is new data waiting to
    // be read from the intermodule communication bus.
    // We need to set up a handler for this event.
    // irq_set_exclusive_handler(PIO0_IRQ_0, isr_serbus);

    // With the handler for the PIO configured, we can
    // go ahead and enable the interrupt
    // irq_set_enabled(PIO0_IRQ_0, true);

    // Startup the second core to allow high-speed
    // response times to the intermodule communication bus.
    // This could be done on core 0 but since core 1 is free,
    // we can use it to (probably marginally) reduce
    // response delay.
    multicore_launch_core1(intermodule_command_handler);

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
    }
}


/**
 * Reset the sequencer grid
 */
void clear_beats(void)
{
    // 16 beats
    for (size_t i = 0; i < 16; ++i) {
        beats[i].ubeat = 0;
        beats[i].veloc = 0;
        beats[i].sample = 0;
    }
}


/**
 * Read the analog value of the variability pot and return a numberically-
 * binned value corresponding to it.
 */
unsigned int read_variability_pot(void)
{
    // TODO!
    return 0;
}


/**
 * Handle serial communication with sequencer.
 * Called when the CBUS_DRDY line is strobed
 */
// void isr_serbus(void)
// {
//     // We need to clear the bits in the interrupt flag
//     // register to acknowledge the interrupt. To do this,
//     // we can use the active bits as a mask for the ones
//     // that we want to clear
//     hw_clear_bits(&pio0_hw->irq, pio0_hw->irq);

//     // new_bus_command = true;
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
    is_selected = !is_selected;

    // Toggle IRQ line low to signal a change to the sequencer
    set_mod_stat_line(false);

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
int64_t alarm_mod_stat_callback(alarm_id_t id, void *user_data)
{
    set_mod_stat_line(true); // Set back HIGH
    return 0; // Time in us to fire again in the future. 0 disables this
}


/**
 * Set the state of the module status line.
 * 
 * @param state true = release line to go high
 *              false = pull line low
 * @return 
 */
void set_mod_stat_line(bool state)
{
    if (state) {
        gpio_set_dir(MOD_STAT_IRQ, GPIO_IN);
    } else {
        gpio_set_dir(MOD_STAT_IRQ, GPIO_OUT);
    }
}


/**
 * Gets started on core 1. The sole task of this is to
 * read and respond to intermodule communication commands.
 */
void intermodule_command_handler(void)
{
    ctlword_t cmd;
    while (true) {
        // Wait for data then read it
        cmd.raw = pio_sm_get_blocking(intermodule_pio.pio, intermodule_pio.sm);

        if (cmd.data.cs) {
            execute_intermodule_command(cmd);
        }
    }        
}


/*
 * Execute a comand based on the control word for it.
 * NOTE: This ignores the "chip select" bit and always
 * executes the operation.
 * NOTE: Under "normal" circumstances this function 
 * will not block. It may, however, block if too many
 * "read" ops are executed before the sequencer completes
 * its read oerations. In this case, if the TX FIFO is
 * filled, this will block until it is no longer full.
 * 
 * @param cmd The control word to execute
 */
void execute_intermodule_command(ctlword_t cmd)
{
    // Write operations
    if (cmd.data.rwb == 0) {
        beats[cmd.data.beat].ubeat = cmd.data.ubeat;
        beats[cmd.data.beat].veloc = cmd.data.veloc;
        beats[cmd.data.beat].sample = cmd.data.sample;
    }
    // Read operations
    else {
        ctlword_t res;
        res.data.beat = cmd.data.beat;
        res.data.ubeat = beats[cmd.data.beat].ubeat;
        res.data.veloc = beats[cmd.data.beat].veloc;
        res.data.sample = beats[cmd.data.beat].sample;
        res.data.modsel = true; // is_selected; // FIXME
        pio_sm_put_blocking(intermodule_pio.pio, intermodule_pio.sm, res.raw);
    }

    // Handle forced module deselection
    if (cmd.data.fdesel == 1) {
        is_selected = false;
    } else {
        is_selected = cmd.data.modsel;
    }
}



