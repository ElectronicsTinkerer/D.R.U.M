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
#include "pico/binary_info.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "Adafruit_Lib/Adafruit_NeoTrellis.h"
#include "Adafruit_Lib/seesaw_neopixel.h"
#include "Adafruit_Lib/Adafruit_NeoTrellis.h"
#include "Adafruit_Lib/Arduino.h"
#include "Adafruit_Lib/Adafruit_SSD1306.h"
#include "adafruit_Lib/Adafruit_GFX.h"

// Fonts
#include "Adafruit_Lib/Fonts/FreeMonoBoldOblique12pt7b.h"

#include "sequencer.pio.h"
#include "serbus.h"
#include "sequencer.h"

time_sig_t time_signatures[] = {
    {3, 4, 12},
    {4, 4, 16},
    {7, 8, 14}
};

#define TSIG_MAX (sizeof(time_signatures)/sizeof(*time_signatures))


// All of these can be modified by the ISRs
volatile bool is_running;
volatile bool is_mod_selected; // Indicates that a module's pattern is currently in the beats array
volatile bool selected_mod_changed;
volatile int bpm;
volatile bool has_bpm_changed;
volatile unsigned int time_sig;
volatile unsigned int current_beat;
volatile signed int current_ubeat;
volatile unsigned int max_beat;
volatile repeating_timer_t timer;
volatile beat_data_t beats[16];
queue_t button_event_queue;
mutex_t mod_sel_mutex;

Adafruit_NeoTrellis trellis;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int main ()
{
    // Hardware init
    stdio_init_all();

    // Init I2C for the screen and keypad
    gpio_set_function(KEYPAD_SDA, GPIO_FUNC_I2C);
    gpio_set_function(KEYPAD_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(KEYPAD_SDA);
    gpio_pull_up(KEYPAD_SCL);

    // Output pins
    gpio_init(GPIO_CBUS_DRDY);
    gpio_set_dir(GPIO_CBUS_DRDY, GPIO_OUT);

    // Inputs
    gpio_init(TEMPO_ENC0);
    gpio_init(TEMPO_ENC1);
    gpio_init(PLYSTP_BTN);
    gpio_init(TSIG_ENC0);
    gpio_init(TSIG_ENC1);
    gpio_init(CLEAR_BTN);
    gpio_set_dir(TEMPO_ENC0, GPIO_IN);
    gpio_set_dir(TEMPO_ENC1, GPIO_IN);
    gpio_set_dir(PLYSTP_BTN, GPIO_IN);
    gpio_set_dir(TSIG_ENC0, GPIO_IN);
    gpio_set_dir(TSIG_ENC1, GPIO_IN);
    gpio_set_dir(CLEAR_BTN, GPIO_IN);

    gpio_init(MOD_STAT_IRQ);
    gpio_set_dir(MOD_STAT_IRQ, GPIO_IN);
    gpio_pull_up(MOD_STAT_IRQ);

    // PIO state machines for intermodule communication (IMC)
    // NOTE: the TX and RX programs MUST be run on separate
    // state machines since each requires an 8-word deep FIFO
    uint tx_offs = pio_add_program(pio0, &seq_tx_program);
    seq_tx_init(pio0, 0, tx_offs, GPIO_CBUS_SCK, GPIO_CBUS_SDOUT, PIO_SPEED_HZ);

    uint rx_offs = pio_add_program(pio1, &seq_rx_program);
    seq_rx_init(pio0, 0, rx_offs, GPIO_CBUS_SCK, GPIO_CBUS_SDOUT, PIO_SPEED_HZ);
    
    gpio_init(GPIO_CBUS_DRDY);
    gpio_set_dir(GPIO_CBUS_DRDY, GPIO_OUT);

    // Initialize Sequencer states
    is_running = false;
    is_mod_selected = false;
    selected_mod_changed = false;
    has_bpm_changed = false;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    max_beat = time_signatures[time_sig].max_ticks;
    current_beat = max_beat;
    current_ubeat = 0;

    // Startup CORE 1 to handle the IMC
    multicore_launch_core1(module_data_controller);
    mutex_init(&mod_sel_mutex);
    queue_init(&button_event_queue);

    // Delay for power supply OLED issues
    sleep_ms(250);
    
    // Casually ignoring the return value
    trellis.begin(NEO_TRELLIS_ADDR, -1);
    
    // CAPVCC = gen drive voltage from 3V3 pin
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.setFont(&FreeMonoBoldOblique12pt7b);
    display.setTextColor(WHITE);

    // Show splash screen
    display.display();    
    
    // Activate all keys and set callbacks
    for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
        trellis.registerCallback(i, isr_pad_event);
    }

    // Do a little animation to show we're on
    for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
        trellis.pixels.setPixelColor(i, 255, 255, 255);
        trellis.pixels.show();
        sleep_ms(50);
    }
    for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
        trellis.pixels.setPixelColor(i, 0x000000);
        trellis.pixels.show();
        sleep_ms(50);
    }

    // Clear display
    display.clearDisplay();
    display.display();

    // Register ISRs to GPIO and Timer events
    // Tempo Encoder
    gpio_set_irq_enabled_with_callback(
        TEMPO_ENC0,
        GPIO_IRQ_EDGE_FALL,
        true, // Enable interrupt
        &isr_gpio_handler
        );

    // Time signature encoder
    gpio_set_irq_enabled(
        TSIG_ENC0,
        GPIO_IRQ_EDGE_FALL,
        true // Enable interrupt
        );

    // Play / pause button
    gpio_set_irq_enabled(
        PLYSTP_BTN,
        GPIO_IRQ_EDGE_FALL,
        true // Enable interrupt
        );

    // Module selection status update
    gpio_set_irq_enabled(
        MOD_STAT_IRQ,
        GPIO_IRQ_EDGE_RISE,
        true // Enable interrupt
        );

    // Main event loop
    while (1) {
        update_screen();
        update_buttons();
        trellis.read();  // interrupt management does all the work! :)
    }
}


/**
 * Updates the screen. Displays the current tempo, time signature,
 * Play/Pause status, and relevant information during module programming
 */
void update_screen(void)
{
    // TODO
    display.clearDisplay();
    display.setCursor(0,18); // Magic numbers for field locations on screen
    display.print(bpm, DEC);
    display.setCursor(84, 18);
    display.print(time_signatures[time_sig].beats, DEC);
    display.print('/');
    display.print(time_signatures[time_sig].measure, DEC);
    display.display();
}


/**
 * Starts the micro beat tick timer
 */
bool start_timer()
{
    // Negative timeout means exact delay (rather than delay between callbacks)
    return add_repeating_timer_us(
        TEMPO_TICK_US_CALC,
        isr_timer,
        NULL,
        &timer
        );
}


/**
 * Stops the micro beat timer
 */
bool stop_timer()
{
    return cancel_repeating_timer(&timer);
}


/**
 * Update the colors of the button matrix based on the current sequencer
 * state.
 */
void update_buttons(void)
{
    for (size_t i = 0; i < 16; ++i) {
        if (is_running && i == current_beat) {
            trellis.pixels.setPixelColor(i, 255, 255, 255);
        }
        else if (i >= max_beat) {
            trellis.pixels.setPixelColor(i, 127, 0, 0); // Light red
        }
        else if (beats[i] == 1) {
            trellis.pixels.setPixelColor(i, 0, 0, 255);
        }
        else {
            trellis.pixels.setPixelColor(i, 0, 0, 0);
        }
    }
    trellis.pixels.show();
}


/**
 * Determines the GPIO which triggered the interrupt and calls it's handler
 */
void isr_gpio_handler(unsigned int gpio, uint32_t event)
{
    switch (gpio) {
    case TEMPO_ENC0:
        isr_tempo_encoder(gpio, event);
        break;

    case PLYSTP_BTN:
        isr_play_pause(gpio, event);
        break;

    case TSIG_ENC0:
        isr_time_sig_encoder(gpio, event);
        break;

    // case CLEAR_BTN:
        // isr_clear_pattern(gpio, event);
        // break;
            
    default:
        return;
    }
}


/**
 * Handles generation of synchronization "tick" pulses with all modules
 */
bool isr_timer(repeating_timer_t *rt)
{
    // bool s = !gpio_get(GPIO_CBUS_DRDY);
    // gpio_put(GPIO_CBUS_DRDY, s);

    // Update the current beat
    if (is_running) {
        if (current_ubeat == 0) {
            ++current_beat;
            if (current_beat >= max_beat) {
                current_beat = 0;
            }
        }
        else if (current_ubeat == MAX_UBEAT) {
            current_ubeat = MIN_UBEAT;
        }
        ++current_ubeat;
    }

    // A bit of a hack, but adjust the timer's timeout value
    // based on the current BPM setting
    if (has_bpm_changed) {
        rt->delay_us = TEMPO_TICK_US_CALC;
        has_bpm_changed = false;
    }
    
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
    mutex_enter_blocking(&mod_sel_mutex); // might not be a great idea in an ISR
    selected_mod_changed = true;
    mutex_exit(&mod_sel_mutex);
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
    if (gpio_get(TEMPO_ENC1) == 1) { // "Forward"
        if (bpm < BPM_MAX) {
            ++bpm;
        }
    } else { // Backward
        if (bpm > BPM_MIN) {
            --bpm;
        }
    }
    has_bpm_changed = true;
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
    if (is_running) {
        current_beat = max_beat;
        current_ubeat = 0;
        start_timer();
    } else {
        stop_timer();
    }
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
    if (gpio_get(TSIG_ENC1) == 1) { // "Forward"
        if (time_sig < TSIG_MAX - 1) {
            ++time_sig;
        } else {
            time_sig = 0;
        }
    } else { // Backward
        if (time_sig > 0) {
            --time_sig;
        } else {
            time_sig = TSIG_MAX - 1;
        }
    }
    max_beat = time_signatures[time_sig].max_ticks;
}


/**
 * Handle a button press on the squish pad
 * 
 * @param The GPIO pin number
 * @param The GPIO event type
 */
TrellisCallback isr_pad_event(keyEvent evt)
{
    // TODO
    // push the event into the button_event_queue
    return 0; // Does something...
}


/**
 * Runs on CORE1 (the second core)
 * Handles sending data to modues and reading data
 * back from the modules.
 */
void module_data_controller(void)
{
    beat_update_t msg;
    while (true) {
        // Try to read the module status change
        // It's OK if we can't for a few tries
        if (mutex_try_enter(&mod_sel_mutex) &&
            queue_is_empty(&button_event_queue))
        {
            if (selected_mod_changed) {
                // TODO: handle writing of all pending data
                // TODO: read in new module's data
                selected_mod_changed = false;
            }
            mutex_exit(&mod_sel_mutex);
        }
        // See if the user pushed a button. If so, update
        // that beat's data
        while (queue_try_remove(&button_event_queue, &msg)) {
            // TODO: handle message
            // TODO: add another mutex for locking of beat array data
        }
    }
}




