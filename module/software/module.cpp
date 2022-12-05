/**
 * MODULE CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */

#include <math.h>

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "samples.h"
#include "module.pio.h"
#include "serbus.h"
#include "module.h"

volatile bool is_selected;
volatile bool dac_loaded; // True following a DAC load event, false otherwise
volatile int current_beat;
volatile int playback_beat;
volatile size_t sample_index;
unsigned int variability;

repeating_timer_t sample_timer;

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
    // gpio_init(GPIO_DAC_SCK);
    // gpio_init(GPIO_DAC_SDAT);
    gpio_set_dir(GPIO_DAC_LDACB, GPIO_OUT);
    gpio_set_dir(GPIO_DAC_CSB, GPIO_OUT);
    gpio_put(GPIO_DAC_LDACB, true);
    gpio_put(GPIO_DAC_CSB, true); // Active low, so disable DAC selection upon RESET
    gpio_set_function(GPIO_DAC_SCK, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_DAC_SDAT, GPIO_FUNC_SPI);
    
    // Input pins
    gpio_init(GPIO_SEL_BTN);
    gpio_init(MOD_TEMPO_SYNC);
    gpio_set_dir(GPIO_SEL_BTN, GPIO_IN);
    gpio_set_dir(MOD_TEMPO_SYNC, GPIO_IN);

    // The "Hey, I've changed status" line
    gpio_init(MOD_STAT_IRQ);
    gpio_put(MOD_STAT_IRQ, 0);
    gpio_set_dir(MOD_STAT_IRQ, GPIO_IN);
    
    // Init
    is_selected = false;
    variability = 0;
    dac_loaded = true;
    current_beat = 0;
    playback_beat = 0;
    sample_index = SAMPLE_LENGTH;
    clear_beats();

    // Set up DAC SPI communication.
    // DAC is write-only
    spi_init(spi0, SPI_DAC_DATA_RATE_HZ);
    spi_set_format(
        spi0,
        16, // Number of bits, defined by DAC
        SPI_CPOL_1,
        SPI_CPHA_1,
        SPI_MSB_FIRST
        );

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

    
    gpio_set_irq_enabled(
        MOD_TEMPO_SYNC,
        GPIO_IRQ_EDGE_FALL,
        true // (enable isr)
        );

    // Set up 44100 KHz sample clock
    // Again, ignoring the return value from this... Might need to fix in future
    add_repeating_timer_us(
        -1000000 / AUDIO_SAMPLE_RATE_HZ,
        isr_sample,
        NULL,
        &sample_timer
        );
    

    uint16_t m;
    int veloc;
    int beat;
    
    // Event loop
    while (1) {
        variability = read_variability_pot();

        if (dac_loaded) {
            dac_loaded = false;

            // Get the value so that we don't need a mutex to prevent it
            // from changing during sample processing
            beat = playback_beat;
            
            // TODO: To add variability, put it in the lowest 1 bits
            veloc = ((beats[beat].veloc & 0x7) << 2);

            if (beats[beat].veloc && sample_index < SAMPLE_LENGTH) {
                ++sample_index;
                m = samples[veloc][sample_index] + 0x8000;
            } else {
                m = 0x8000; // Midscale
            }
            
            // Enable the DAC
            gpio_put(GPIO_DAC_CSB, false);

            // Send the new value
            spi_write16_blocking(
                spi0,
                &m,
                1
                );

            // Disable the DAC
            gpio_put(GPIO_DAC_CSB, true);

            // Note: This does not actually load the DAC!!!
            // This only fills up the shift register with the
            // next beat's sample data. The tempo sync ISR will
            // handle loading of the DAC.
        }
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

    case MOD_TEMPO_SYNC:
        isr_tempo_sync(gpio, event);
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
 * @param id <unused>
 * @param *user_data <unused>
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
 * Triggered on a change on the tempo sync line. Causes the DAC
 * to load in the value currently in its shift register
 * 
 * @param gpio <unused>
 * @param even <unused>
 */
void isr_tempo_sync(unsigned int gpio, uint32_t event)
{
    // Callback in 100 us to resample the GPIO line
    // This allows us to detect beat 0 (long low value)
    add_alarm_in_us(ALARM_TEMPO_SAMPLE_US, alarm_mod_beat_callback, NULL, false);
}


/**
 * Handles detection of the first beat (beat 0) to synchronize with
 * the sequencer
 */
int64_t alarm_mod_beat_callback(alarm_id_t id, void *user_data)
{
    if (gpio_get(MOD_TEMPO_SYNC) == 1) {
        current_beat = 0;
    } else {
        ++current_beat;
        current_beat %= 16; // FIXME
    }

    // Only restart sample playback if there is a note this beat.
    // Otherwise, let it ring out to end
    if (beats[current_beat].veloc != 0) {
        sample_index = 0;
        playback_beat = current_beat;
    }
    
    return 0; // Time in us to fire again in the future. 0 disables this
}


/**
 * Called by the sample rate timer. Handles loading of data to the DAC.
 * Note that this does not perform any data shifting to the DAC. It
 * simply tells the DAC to load what has already been shifted in.
 */
bool isr_sample(repeating_timer_t *t)
{
    gpio_put(GPIO_DAC_LDACB, false);
    // Datasheet says pulse must > 40ns. This makes it about 80ns
    asm volatile("nop \n nop \n nop"); // Make the pulse 'wider'
    asm volatile("nop \n nop \n nop"); // Make the pulse 'wider'
    asm volatile("nop \n nop \n nop"); // Make the pulse 'wider'
    gpio_put(GPIO_DAC_LDACB, true);
    dac_loaded = true; // Signal to main thread that the dac was loaded
    
    return true; // Keep repeating
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

        execute_intermodule_command(cmd);
    }        
}


/*
 * Execute a comand based on the control word for it.
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
    // Don't execute the command is this module is not selected
    if (!cmd.data.cs) {
        return;
    }

    // Write operations
    if (cmd.data.rwb == 0) {
        beats[cmd.data.beat].ubeat = cmd.data.ubeat;
        beats[cmd.data.beat].veloc = cmd.data.veloc;
    }
    // Read operations
    else { // FIXME!!!!!
        ctlword_t res;
        res.raw = 0;
        res.data.beat = 0xa;//cmd.data.beat;
        res.data.ubeat = 0x6;//beats[cmd.data.beat].ubeat;
        res.data.veloc = beats[cmd.data.beat].veloc;
        res.data.modsel = true; // is_selected; // FIXME
        pio_sm_put_blocking(intermodule_pio.pio, intermodule_pio.sm, 0xff00aa55);
    }

    // Handle forced module deselection
    if (cmd.data.fdesel == 1) {
        is_selected = false;
    } else {
        is_selected = cmd.data.modsel;
    }
}



