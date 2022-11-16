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
#include "Adafruit_Lib/seesaw_neopixel.h"
#include "Adafruit_Lib/Adafruit_NeoTrellis.h"
#include "Adafruit_Lib/Arduino.h"
#include "Adafruit_Lib/Adafruit_SSD1306.h"
#include "adafruit_Lib/Adafruit_GFX.h"

// Fonts
#include "Adafruit_Lib/Fonts/FreeMonoBoldOblique12pt7b.h"

#include "serbus.h"
#include "sequencer.h"

time_sig_t time_signatures[] = {
    {3, 4, 12},
    {4, 4, 16},
    {7, 8, 14}
};


// All of these can be modified by the ISRs
volatile bool is_running;
volatile bool is_mod_selected;
volatile int bpm;
volatile bool has_bpm_changed;
volatile unsigned int time_sig;
volatile unsigned int current_beat;
volatile signed int current_ubeat;
volatile unsigned int max_beat;
repeating_timer_t timer;
volatile uint8_t beats[16]; // TODO: Make beats a struct

Adafruit_NeoTrellis trellis;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int main ()
{
    // Hardware init
    stdio_init_all();

    // Init I2C for the screen and keypad
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Output pins
    gpio_init(GPIO_CBUS_DRDY);
    gpio_set_dir(GPIO_CBUS_DRDY, GPIO_OUT);

    // Inputs
    gpio_init(TEMPO_ENC0);
    gpio_init(TEMPO_ENC1);
    gpio_init(PLYSTP_BTN);
    gpio_set_dir(TEMPO_ENC0, GPIO_IN);
    gpio_set_dir(TEMPO_ENC1, GPIO_IN);
    gpio_set_dir(PLYSTP_BTN, GPIO_IN);

    // Initialize Sequencer states
    is_running = true;
    is_mod_selected = false;
    has_bpm_changed = false;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    current_beat = 0;
    max_beat = time_signatures[time_sig].max_ticks;
    
    trellis.begin(NEO_TRELLIS_ADDR, -1); // Casually ignoring the return value
    // CAPVCC = gen drive voltage from 3V3 pin
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.setFont(&FreeMonoBoldOblique12pt7b);
    display.setTextColor(WHITE);

    // Show splash screen
    display.display();    
    
    //activate all keys and set callbacks
    for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
        trellis.registerCallback(i, isr_pad_event);
    }

    // do a little animation to show we're on
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
        &isr_tempo_encoder
        );

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

    // Play / pause button
    // gpio_set_irq_enabled_with_callback(
        // PLYSTP_BTN,
        // GPIO_IRQ_EDGE_FALL,
        // true, // Enable interrupt
        // &isr_play_pause
        // );

    // // Module selection status update
    // gpio_set_irq_enabled_with_callback(
    //     MOD_STAT_IRQ,
    //     GPIO_IRQ_EDGE_RISE,
    //     true, // Enable interrupt
    //     &isr_module_status
    //     );

    // Set up the tick timer for micro beat generation    
    
    // Negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(
            TEMPO_TICK_US_CALC,
            isr_timer,
            NULL,
            &timer)
        ) {
        // printf("Failed to add timer\n");
        // ERROR!
        return 1;
    }


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
    display.println(bpm, DEC);
    display.display();
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
        else if (beats[i] == 1) {
            trellis.pixels.setPixelColor(i, 0, 0, 255);
        } else {
            trellis.pixels.setPixelColor(i, 0, 0, 0);
        }
    }
    trellis.pixels.show();
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
        current_beat = 0;
        current_ubeat = 0;
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
    return 0; // Does something...
}



