/**
 * SEQUENCER CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */


#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "Adafruit_Lib/Adafruit_NeoTrellis.h"

#include "serbus.h"
// #include "oled.h"
#include "sequencer.h"

// All of these can be modified by the ISRs
volatile unsigned int is_running;
volatile unsigned int is_mod_selected;
volatile int bpm;
volatile unsigned int time_sig;
volatile unsigned int current_beat;
repeating_timer_t timer;

// Adafruit_NeoTrellis keys = Adafruit_NeoTrellis();

int main ()
{
    // Hardware init
    stdio_init_all();

    // Output pins
    gpio_init(GPIO_CBUS_DRDY);
    gpio_set_dir(GPIO_CBUS_DRDY, GPIO_OUT);

    // Initialize Sequencer states
    is_running = false;
    is_mod_selected = false;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    current_beat = 0;

    // // BEGIN RPI
    // // I2C is "open drain", pull ups to keep signal high when no data is being
    // // sent
    // i2c_init(i2c_default, 400 * 1000);
    // gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    // gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    // gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    // gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // // run through the complete initialization process
    // oled_init();

    // // initialize render area for entire frame (128 pixels by 4 pages)
    // render_area_t frame_area = {start_col: 0, end_col : OLED_WIDTH - 1, start_page : 0, end_page : OLED_NUM_PAGES - 1};
    // calc_render_area_buflen(&frame_area);

    // // zero the entire display
    // uint8_t buf[OLED_BUF_LEN];
    // fill(buf, 0x00);
    // render(buf, &frame_area);

    // // intro sequence: flash the screen 3 times
    // for (int i = 0; i < 3; i++) {
    //     oled_send_cmd(0xA5); // ignore RAM, all pixels on
    //     sleep_ms(500);
    //     oled_send_cmd(0xA4); // go back to following RAM
    //     sleep_ms(500);
    // }
    // // END RPI
    
    // // Register ISRs to GPIO and Timer events
    // // Tempo Encoder
    // gpio_set_irq_enabled_with_callback(
    //     TEMPO_ENC0,
    //     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    //     true, // Enable interrupt
    //     &isr_tempo_encoder
    //     );
    // gpio_set_irq_enabled_with_callback(
    //     TEMPO_ENC1,
    //     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    //     true, // Enable interrupt
    //     &isr_tempo_encoder
    //     );

    // // Time signature encoder
    // gpio_set_irq_enabled_with_callback(
    //     TSIG_ENC0,
    //     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    //     true, // Enable interrupt
    //     &isr_time_sig_encoder
    //     );
    // gpio_set_irq_enabled_with_callback(
    //     TSIG_ENC1,
    //     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    //     true, // Enable interrupt
    //     &isr_time_sig_encoder
    //     );

    // // Play / pause button
    // gpio_set_irq_enabled_with_callback(
    //     PLYSTP_BTN,
    //     GPIO_IRQ_EDGE_FALL,
    //     true, // Enable interrupt
    //     &isr_play_pause
    //     );

    // // Module selection status update
    // gpio_set_irq_enabled_with_callback(
    //     MOD_STAT_IRQ,
    //     GPIO_IRQ_EDGE_RISE,
    //     true, // Enable interrupt
    //     &isr_module_status
    //     );

    // Set up the tick timer for micro beat generation    
    
    // Negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us((-1000000 * 60) / bpm, isr_timer, NULL, &timer)) {
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
bool isr_timer(repeating_timer_t *rt)
{
    // TODO!
    gpio_put(GPIO_CBUS_DRDY, !gpio_get(GPIO_CBUS_DRDY));
    

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



