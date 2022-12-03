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

// PINS
#define GPIO_TP9         0
#define GPIO_ENC00       1
#define GPIO_ENC01       2
#define GPIO_TP10        3
#define GPIO_I2C0_SDA0   4
#define GPIO_I2C0_SCL0   5
#define GPIO_ENC10       6
#define GPIO_ENC11       7
#define GPIO_ENC0B       8
#define GPIO_ENC2B       9
#define GPIO_ENC20       10
#define GPIO_ENC21       11
#define GPIO_I2C0_SDA1   12 // DISCONNECTED on PCB!
#define GPIO_I2C0_SCL1   13 // DISCONNECTED on PCB!
#define GPIO_ENC30       14
#define GPIO_ENC31       15
#define GPIO_CBUS_EXTRA0 16
#define GPIO_CBUS_DRDY   17
#define GPIO_CBUS_SCK    18
#define GPIO_CBUS_SDIN   19
#define GPIO_CBUS_SDOUT  20
#define GPIO_CBUS_EXTRA1 21
#define GPIO_CBUS_NXTPR  22
#define GPIO_BTN0        23
#define GPIO_BTN1        24
// GPIO 25
#define GPIO_ADC0        26
#define GPIO_ADC1        27
#define GPIO_ADC2        28
#define GPIO_ADC3        29


// Named pin mapping
#define TEMPO_ENC0      GPIO_ENC00
#define TEMPO_ENC1      GPIO_ENC01
#define TSIG_ENC0       GPIO_ENC20
#define TSIG_ENC1       GPIO_ENC21
#define PLYSTP_BTN      GPIO_ENC0B
#define CLEAR_BTN       GPIO_ENC2B
#define MOD_STAT_IRQ    GPIO_CBUS_EXTRA0
#define KEYPAD_SDA      GPIO_I2C0_SDA0
#define KEYPAD_SCL      GPIO_I2C0_SCL0


// Tempo settings
#define BPM_MIN 20
#define BPM_DEFAULT 120
#define BPM_MAX 400
#define TICKS_PER_ROW 4 // Number of pads per beat (must be a power of 2)
#define TOTAL_UBEATS 9
#define MIN_UBEAT (-(TOTAL_UBEATS-1)/2) // Microbeats before the beat
#define MAX_UBEAT ((TOTAL_UBEATS-1)/2)  // Microbeats after the beat

#define TEMPO_TICK_US_CALC ((-1000000 * 60) / (bpm * TOTAL_UBEATS * TICKS_PER_ROW))

// Time signature settings
#define TS_DEFAULT 1

typedef struct time_sig_t {
    uint8_t beats;     // Numberator of time signature
    uint8_t measure;   // Denominator of time signature
    uint8_t max_ticks; // Number of pads to use
} time_sig_t;


// PIO Tx/Rx config
#define PIO_SPEED_HZ ((float)1000000)

// Number of entries in the queue between core 0 and core 1
// This queue stores the updates that the user has entered on core 0
// Then core 1 reads the updates, applies them to the main 'beats'
// array and also pushes the change to the currently selected module
#define BEAT_DATA_QUEUE_LEN 16

typedef struct beat_data {
    signed int ubeat    : 4;
    unsigned int veloc  : 4; // 0 = no hit
    unsigned int sample : 4;
} beat_data_t;

typedef enum {
    CHANGE_VELOC,
    CLEAR_GRID
} update_type_t;

typedef struct beat_update {
    update_type_t op;
    unsigned int beat;
    int delta_veloc; // 0 = no hit
} beat_update_t;


#define MAX_VELOCITY 8

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3D // Magic number from OLED datasheet

// Function signatures
bool start_timer();
bool stop_timer();
void update_screen(void);
void update_buttons(void);
void isr_gpio_handler(unsigned int, uint32_t);
bool isr_timer(repeating_timer_t *);
void isr_module_status(unsigned int, uint32_t);
void isr_tempo_encoder(unsigned int, uint32_t);
void isr_play_pause(unsigned int, uint32_t);
void isr_time_sig_encoder(unsigned int, uint32_t);
TrellisCallback isr_pad_event(keyEvent);
void isr_clear_pattern(unsigned int, uint32_t);
void module_data_controller(void);
void handle_beat_data_change(beat_update_t *);
int get_connected_module_count(void);
void serbus_txrx(uint32_t *, uint32_t *, size_t);

#endif

