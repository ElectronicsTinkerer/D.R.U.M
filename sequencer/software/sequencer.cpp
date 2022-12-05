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
#include "pico/critical_section.h"
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
volatile int bpm;
volatile bool has_bpm_changed;
volatile unsigned int time_sig;
volatile unsigned int current_beat;
volatile signed int current_ubeat;
volatile unsigned int max_beat;
repeating_timer_t timer;
critical_section_t oled_spinlock;
int connected_module_count;

struct {
    volatile bool has_mod_irq;
    volatile beat_data_t beats[16];
    int last_veloc; // The result of the last velocity change
    volatile bool do_screen_update;
    volatile bool is_mod_selected; // Indicates that a module's pattern is currently in the beats array
    int selected_mod_index;

    queue_t queue_button_event;

    mutex_t mutex_has_mod_irq;
    mutex_t mutex_beats;
    mutex_t mutex_last_veloc;
    mutex_t mutex_do_screen_update;
} mod_data;


struct {
    uint tx_sm;
    PIO tx_pio;
    uint tx_offs;
    uint rx_sm;
    PIO rx_pio;
    uint rx_offs;
} intermodule_pio;

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
    gpio_put(GPIO_CBUS_DRDY, 1); // Initially not transmitting. This signal is inverted

    // This clock line will be reinitialied within the PIO TX init
    // It needs to be toggled to reset the module's state machines
    // gpio_init(GPIO_CBUS_SCK);
    // gpio_set_dir(GPIO_CBUS_SCK, GPIO_OUT);
    // gpio_put(GPIO_CBUS_SCK, 1);
    // gpio_put(GPIO_CBUS_SCK, 0);


    // Inputs
    gpio_init(TEMPO_ENC0);
    gpio_init(TEMPO_ENC1);
    gpio_init(PLYSTP_BTN);
    gpio_init(TSIG_ENC0);
    gpio_init(TSIG_ENC1);
    gpio_init(CLEAR_BTN);
    gpio_init(GPIO_CBUS_NXTPR);
    gpio_set_dir(TEMPO_ENC0, GPIO_IN);
    gpio_set_dir(TEMPO_ENC1, GPIO_IN);
    gpio_set_dir(PLYSTP_BTN, GPIO_IN);
    gpio_set_dir(TSIG_ENC0, GPIO_IN);
    gpio_set_dir(TSIG_ENC1, GPIO_IN);
    gpio_set_dir(CLEAR_BTN, GPIO_IN);
    gpio_set_dir(GPIO_CBUS_NXTPR, GPIO_IN);

    gpio_init(MOD_STAT_IRQ);
    gpio_set_dir(MOD_STAT_IRQ, GPIO_IN);
    gpio_pull_up(MOD_STAT_IRQ);

    // PIO state machines for intermodule communication (IMC)
    // NOTE: the TX and RX programs MUST be run on separate
    // state machines since each requires an 8-word deep FIFO
    intermodule_pio.tx_pio = pio0;
    intermodule_pio.tx_sm = pio_claim_unused_sm(intermodule_pio.tx_pio, true);
    intermodule_pio.tx_offs = pio_add_program(intermodule_pio.tx_pio, &seq_tx_program);
    seq_tx_init(
        intermodule_pio.tx_pio,
        intermodule_pio.tx_sm,
        intermodule_pio.tx_offs,
        GPIO_CBUS_SCK,
        GPIO_CBUS_SDOUT
        );

    intermodule_pio.rx_pio = pio1;
    intermodule_pio.rx_sm = pio_claim_unused_sm(intermodule_pio.rx_pio, true);
    intermodule_pio.rx_offs = pio_add_program(intermodule_pio.rx_pio, &seq_rx_program);
    seq_rx_init(
        intermodule_pio.rx_pio,
        intermodule_pio.rx_sm,
        intermodule_pio.rx_offs,
        GPIO_CBUS_SCK,
        GPIO_CBUS_SDIN,
        GPIO_CBUS_DRDY
        );


    // Initialize Sequencer states
    is_running = false;
    mod_data.is_mod_selected = true; //false; // DEBUG  ************************************
    mod_data.has_mod_irq = false;
    mod_data.do_screen_update = true;
    mod_data.last_veloc = 0;
    mod_data.selected_mod_index = 0;
    has_bpm_changed = false;
    bpm = BPM_DEFAULT;
    time_sig = TS_DEFAULT;
    max_beat = time_signatures[time_sig].max_ticks;
    current_beat = max_beat;
    current_ubeat = 0;
    connected_module_count = get_connected_module_count();

    critical_section_init(&oled_spinlock);

    // Startup CORE 1 to handle the IMC
    multicore_launch_core1(module_data_controller);
    mutex_init(&mod_data.mutex_has_mod_irq);
    mutex_init(&mod_data.mutex_beats);
    mutex_init(&mod_data.mutex_last_veloc);
    mutex_init(&mod_data.mutex_do_screen_update);
    queue_init(
        &mod_data.queue_button_event,
        sizeof(beat_update_t),
        BEAT_DATA_QUEUE_LEN
        );

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

    // Clear beat pad button
    gpio_set_irq_enabled(
        CLEAR_BTN,
        GPIO_IRQ_EDGE_FALL,
        true // Enable interrupt
        );

    // Module selection status update
    gpio_set_irq_enabled(
        MOD_STAT_IRQ,
        GPIO_IRQ_EDGE_RISE,
        true // Enable interrupt
        );

    uint32_t tx[] = {
        (const uint32_t)0xaaaaaaaa,
        (const uint32_t)0xffffffff,
        (const uint32_t)0x80808080,
        (const uint32_t)0xc3c3c3c3
    };

    uint32_t rx[8];

    // Main event loop
    while (1) {
        update_screen();
        trellis.read();  // interrupt management does all the work! :)
        update_buttons();
        sleep_ms(20);
        // intermodule_serbus_txrx(
        //     intermodule_pio.rx_pio,
        //     intermodule_pio.rx_sm,
        //     intermodule_pio.tx_pio,
        //     intermodule_pio.tx_sm,
        //     rx,
        //     tx,
        //     4,
        //     GPIO_CBUS_DRDY,
        //     GPIO_CBUS_SCK
        //     );
        // for (size_t i = 0; i < 3; ++i) {
        //     if (tx[i] != rx[i+1]) {
        //         is_running = false;
        //     }
        // }
    }
}


/**
 * Updates the screen. Displays the current tempo, time signature,
 * Play/Pause status, and relevant information during module programming
 */
void update_screen(void)
{
    uint32_t owner;
    bool update;

    // Check if there needs to be updates sent to the screen
    critical_section_enter_blocking(&oled_spinlock);
    update = mutex_try_enter(&mod_data.mutex_do_screen_update, &owner);
    mutex_exit(&mod_data.mutex_do_screen_update);
    critical_section_exit(&oled_spinlock);
    if (update) {
        update = mod_data.do_screen_update;
    }

    // If there's updates to do, send them
    if (update) {
        display.clearDisplay();

        // Display BPM
        display.setCursor(0,18); // Magic numbers for field locations on screen
        display.print(bpm, DEC);

        // Display time signature
        display.setCursor(84, 18);
        display.print(time_signatures[time_sig].beats, DEC);
        display.print('/');
        display.print(time_signatures[time_sig].measure, DEC);

        // Display last velocity update
        display.setCursor(0, 60);
        mutex_enter_blocking(&mod_data.mutex_last_veloc);
        display.print(mod_data.last_veloc, DEC);
        mutex_exit(&mod_data.mutex_last_veloc);

        display.setCursor(30, 60);
        display.print(connected_module_count);

        // Send data to display
        display.display();

        // Done with update
        mod_data.do_screen_update = false;
    }
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
    mutex_enter_blocking(&mod_data.mutex_beats);
    for (size_t i = 0; i < 16; ++i) {
        if (is_running && i == current_beat) {
            trellis.pixels.setPixelColor(i, 255, 255, 255);
        }
        else if (i >= max_beat) {
            trellis.pixels.setPixelColor(i, 127, 0, 0); // Light red
        }
        else if (mod_data.beats[i].veloc > 0) {
            trellis.pixels.setPixelColor(i, 30, 127, 127); // A "nice" teal
        }
        else {
            trellis.pixels.setPixelColor(i, 0, 0, 0);
        }
    }
    mutex_exit(&mod_data.mutex_beats);
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

    case CLEAR_BTN:
        isr_clear_pattern(gpio, event);
        break;

    case MOD_STAT_IRQ:
        isr_module_status(gpio, event);
        break;
        
    default:
        return;
    }
}


/**
 * Handles generation of synchronization "tick" pulses with all modules
 */
bool isr_timer(repeating_timer_t *rt)
{
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
    uint32_t owner;
    mutex_try_enter(&mod_data.mutex_has_mod_irq, &owner);
    mod_data.has_mod_irq = true;
    mutex_exit(&mod_data.mutex_has_mod_irq);
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

    // Tell screen to do an update
    mutex_enter_blocking(&mod_data.mutex_do_screen_update);
    mod_data.do_screen_update = true;
    mutex_exit(&mod_data.mutex_do_screen_update);
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
    
    // Tell screen to do an update
    mutex_enter_blocking(&mod_data.mutex_do_screen_update);
    mod_data.do_screen_update = true;
    mutex_exit(&mod_data.mutex_do_screen_update);
}


/**
 * Handle a button press on the squish pad
 */
TrellisCallback isr_pad_event(keyEvent evt)
{
    // On rising edge, send the update only if
    // the user pressed a valid beat for this
    // time signature
    if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING &&
        evt.bit.NUM < max_beat) {
        beat_update_t msg = {
            .op = CHANGE_VELOC,
            .beat = evt.bit.NUM,
            .delta_veloc = 1 // Just increment the velocity
        };
        // Send the event to the other core
        // If the queue is full, ignore the data...
        queue_try_add(&mod_data.queue_button_event, &msg);
    }
        
    return 0; // Does something...
}


/**
 * Send a message to CORE 1 to reset all beat data on the current module
 */
void isr_clear_pattern(unsigned int gpio, uint32_t event)
{
    beat_update_t msg = {
        .op = CLEAR_GRID,
        .beat = 0,
        .delta_veloc = 0
    };    
    queue_add_blocking(&mod_data.queue_button_event, &msg);
}


/**
 * Runs on CORE1 (the second core)
 * Handles sending data to modues and reading data
 * back from the modules.
 */
void module_data_controller(void)
{
    beat_update_t msg;
    uint32_t owner;
    ctlword_t _mods[connected_module_count];
    ctlword_t *mods = _mods;
    size_t i;
    
    while (true) {
        // See if the user pushed a button. If so, update
        // that beat's data
        while (queue_try_remove(&mod_data.queue_button_event, &msg)) {
            handle_beat_data_change(&msg);
        }
        
        // Try to read the module status change
        // It's OK if we can't for a few tries
        if (mutex_try_enter(&mod_data.mutex_has_mod_irq, &owner)) {
            if (queue_is_empty(&mod_data.queue_button_event)) {
                if (mod_data.has_mod_irq) {

                    mod_data.selected_mod_index = 1; // DEBUG

                    // // Get the selection status of all modules
                    // serbus_read_cmd_all(mods);

                    // // Go through the modules and find the first one
                    // // which is selected
                    // for (i = 0; i < connected_module_count && !mods[i].data.modsel; ++i) {
                    //     /* loop */
                    // }
                    // if (mods[i].data.modsel) {
                    //     serbus_select_module(i);
                    //     // Signal that a module is selected
                    //     mod_data.is_mod_selected = true;
                    //     mod_data.selected_mod_index = i;
                    // } else {
                    //     serbus_select_module(-1); // Deselect all
                    //     // Signal that no modules are selected
                    //     mod_data.is_mod_selected = false;
                    //     mod_data.selected_mod_index = 0;
                    // }
                    
                    // // TODO: read in new module's data
                    mod_data.has_mod_irq = false;
                }
            }
            mutex_exit(&mod_data.mutex_has_mod_irq);
        }

    }
}


/**
 * Handles updates to the beat array and synchronization of that data
 * with the modules
 */
void handle_beat_data_change(beat_update_t *msg)
{
    size_t i;
    ctlword_t cmd;
   
    // Check to make sure that a module is actually selected
    // before updating any beat data
    if (!mod_data.is_mod_selected) {
        return;
    }
                
    // Handle message
    // Wait until able to access beat array, then update it
    mutex_enter_blocking(&mod_data.mutex_beats);

    // Determine what op this wants to do and do it
    switch (msg->op) {
    case CHANGE_VELOC:
        i = mod_data.beats[msg->beat].veloc + msg->delta_veloc; // Appply relative change
        i %= MAX_VELOCITY;
        mod_data.beats[msg->beat].veloc = i;

        // Tell screen to do an update
        mutex_enter_blocking(&mod_data.mutex_do_screen_update);
        mod_data.do_screen_update = true;
        mutex_exit(&mod_data.mutex_do_screen_update);
        // Update the last velocity value
        mutex_enter_blocking(&mod_data.mutex_last_veloc);
        mod_data.last_veloc = i;
        mutex_exit(&mod_data.mutex_last_veloc);

        // Send back modified beat data to module
         cmd.raw = 0;
        cmd.data.beat = msg->beat;
        cmd.data.ubeat = mod_data.beats[msg->beat].ubeat;
        cmd.data.veloc = mod_data.beats[msg->beat].veloc;
        cmd.data.sample = mod_data.beats[msg->beat].sample;
        cmd.data.cs = 1;
        serbus_write_cmd(mod_data.selected_mod_index, cmd);
        break;

    case CLEAR_GRID:
        // Zero the entire beat grid
        for (i = 0; i < 16; ++i) {
            mod_data.beats[i].veloc = 0;
            mod_data.beats[i].ubeat = 0;
            mod_data.beats[i].sample = 0;
        }

        // Tell screen to do an update
        mutex_enter_blocking(&mod_data.mutex_do_screen_update);
        mod_data.do_screen_update = true;
        mutex_exit(&mod_data.mutex_do_screen_update);
        // Update the last velocity value
        mutex_enter_blocking(&mod_data.mutex_last_veloc);
        mod_data.last_veloc = 0;
        mutex_exit(&mod_data.mutex_last_veloc);

        // Send back a cleared beat array to module
        cmd.raw = 0;
        cmd.data.cs = 1;
        for (i = 0; i < 16; ++i) {
            cmd.data.beat = i;
            serbus_write_cmd(mod_data.selected_mod_index, cmd);
        }

        break;
                
    default:
        break;
    }
    mutex_exit(&mod_data.mutex_beats);
}


/**
 * Perform a scan of the intermodule scan chain and determine how
 * many modules are connected
 * 
 * @return The number of modules connected
 */
int get_connected_module_count(void)
{
    // Can't scan for 0 modules, but we can check if the
    // "Next present" pin is low (it is pulled down when
    // no module is present)
    if (!gpio_get(GPIO_CBUS_NXTPR)) {
        return 0;
    }

    // We need a magic number that does not execute
    // commands on the module. That's what this is.
    // (Any word with the upper 4 bits zeroed will
    // work for this task.) The code below
    // sends out the magic number and waits for it
    // to return. It counts the number of words read
    // in and uses that as the number of modules
    // that are connected
    uint32_t pattern = 0x02dac48f;
    uint32_t rx[8]; // Max of 8 modules can be connected (HW limitation)

    size_t i = 0;

    for (i = 0; i < 8; ++i) {
        rx[i] = 0;
    }

    // Flush the scan chain
    serbus_txrx(&rx[0], NULL, 8);

    i = 0;
    
    // Using 9 as the "max" since only 8 modules
    // are supported in hardware
    while (rx[0] != pattern && i < 9) {
        serbus_txrx(&pattern, rx, 1);
        ++i;
    }

    // Error condition, IDK what happened
    if (i == 9) {
        return 0;
    }
    
    return i-1;
}


/**
 * Send a command to a specific module, ignoring the data
 * which gets shifted back during the write cycle
 * NOTE: There is no error checking on the module index.
 * 
 * @param mod_index The index of the module to send data to.
 * @param cmd The command to send to the module
 */
void serbus_write_cmd(size_t mod_index, ctlword_t cmd)
{
    if (connected_module_count == 0) {
        return;
    }
    
    ctlword_t cmds[connected_module_count];
    size_t i;

    // Reset the control words to be sent
    // While not strictly necessary, it is good to be safe
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
    }

    // Copy in the data to be sent
    cmds[mod_index].raw = cmd.raw;
    cmds[mod_index].data.cs = 1;
    cmds[mod_index].data.rwb = 0; // write

    // Send the command
    serbus_txrx(&cmds[0].raw, NULL, connected_module_count);
}


/**
 * Send a command to read back data from a specific module.
 * NOTE: there is no error checking on the module index.
 * 
 * @param mod_index The index of the module to read from
 * @param cmd The command to send to the module
 * @param *response The response from the module
 */
void serbus_read_cmd(size_t mod_index, ctlword_t cmd, ctlword_t *response)
{
    if (connected_module_count == 0) {
        return;
    }

    ctlword_t cmds[connected_module_count];
    size_t i;

    // Reset the control words to be sent
    // While not strictly necessary, it is good to be safe
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
    }

    // Copy in the data to be sent
    cmds[mod_index].raw = cmd.raw;
    cmds[mod_index].data.cs = 1;
    cmds[mod_index].data.rwb = 1; // read

    // Send the command
    serbus_txrx(&cmds[0].raw, NULL, connected_module_count);

    // There's no acknowledge on the module communication
    // so sleep for a bit and *hope* the module is ready...
    sleep_us(100);

    // Reset the request commands
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
    }

    // Get the response from the module
    serbus_txrx(&cmds[0].raw, &cmds[0].raw, connected_module_count);
    
    response->raw = cmds[mod_index].raw;
}


/**
 * Send a command to read back data from all connected modules
 * NOTE: there is no error checking on the module index.
 * 
 * @param *response The response from the modules
 */
void serbus_read_cmd_all(ctlword_t *response)
{
    if (connected_module_count == 0) {
        return;
    }

    ctlword_t cmds[connected_module_count];
    size_t i;

    // Reset the control words to be sent
    // While not strictly necessary, it is good to be safe
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
        cmds[i].data.cs = 1;
        cmds[i].data.rwb = 1;
    }

    // Send the command
    serbus_txrx(&cmds[0].raw, NULL, connected_module_count);

    // There's no acknowledge on the module communication
    // so sleep for a bit and *hope* the module is ready...
    sleep_us(10);

    // Reset the request commands
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
    }

    // Get the response from the module
    serbus_txrx(&cmds[0].raw, &response->raw, connected_module_count);
}


/**
 * Select a single module by index. Forces all other modules to deselect
 * NOTE: There is no error checking on the module index.
 * 
 * @param mod_index The module to select. -1 deselects all modues
 */
void serbus_select_module(int mod_index)
{
    if (connected_module_count == 0) {
        return;
    }
    
    ctlword_t cmds[connected_module_count];
    size_t i;

    // Reset the control words to be sent
    // While not strictly necessary, it is good to be safe
    for (i = 0; i < connected_module_count; i++) {
        cmds[i].raw = 0;
        cmds[i].data.fdesel = 1; // force deselection
        cmds[i].data.cs = 1; // Execute the command
    }

    if (mod_index >= 0) {
        cmds[mod_index].data.fdesel = 0;
        cmds[mod_index].data.modsel = 1;
    }

    // Send the command
    serbus_txrx(&cmds[0].raw, NULL, connected_module_count);
}


/**
 * Simplified wrapper to send and receive data from the intermodule bus
 * 
 * @param *tx A buffer to send
 * @param *rx A buffer to put received data into
 * @param count The number of words to send/receive
 * @return none
 */
inline void serbus_txrx(uint32_t *tx, uint32_t *rx, size_t count)
{
    intermodule_serbus_txrx(
        intermodule_pio.rx_pio,
        intermodule_pio.rx_sm,
        intermodule_pio.tx_pio,
        intermodule_pio.tx_sm,
        rx,
        tx,
        count,
        GPIO_CBUS_DRDY,
        GPIO_CBUS_SCK
        );
}




