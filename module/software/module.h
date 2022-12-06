/**
 * MODULE CONTROLLER
 * 
 * Authors:
 * Zach Baldwin
 * Andy Regli
 * Mark Vaughn
 *
 */


#ifndef DRUM_MODULE_H
#define DRUM_MODULE_H

// Pin definitions
// GPIO 0
#define GPIO_ENC0        1
#define GPIO_ENC1        2
#define GPIO_ENC2        3
// GPIO 4
// GPIO 5
#define GPIO_CBUS_EXTRA1 6
#define GPIO_CBUS_EXTRA0 7
#define GPIO_CBUS_SDIN   8
#define GPIO_CBUS_DRDY   9
#define GPIO_CBUS_SCK    10
#define GPIO_CBUS_SDOUT  11
#define GPIO_TP7         12
#define GPIO_TP8         13
#define GPIO_TP9         14
// GPIO 15
#define GPIO_DAC_LDACB   16
#define GPIO_DAC_CSB     17
#define GPIO_DAC_SCK     18
#define GPIO_DAC_SDAT    19
#define GPIO_CHAN_FULL   20
// GPIO 21
// GPIO 22
#define GPIO_BTN0        23
#define GPIO_BTN1        24
// GPIO 25
#define GPIO_ADC0        26
#define GPIO_ADC1        27
#define GPIO_ADC2        28
#define GPIO_ADC3        29

// Named pin mappings
#define MOD_TEMPO_SYNC   GPIO_CBUS_EXTRA1
#define MOD_STAT_IRQ     GPIO_CBUS_EXTRA0
#define GPIO_SEL_BTN     GPIO_BTN0

#define SPI_DAC_DATA_RATE_HZ 10000000
#define AUDIO_SAMPLE_RATE_HZ 44100

#define ALARM_TEMPO_SAMPLE_US 75

#define VARIABILITY_STEPS 4 // MUST be a power of 2!
#define VARIABILITY_STEPS_LOG 2 // LOG2(VARIABILITY_STEPS)

typedef struct beat_data {
    signed int ubeat    : 4;
    unsigned int veloc  : 4; // 0 = no hit
    unsigned int sample : 4;
} beat_data_t;

void clear_beats(void);
uint16_t read_variability_pot(void);
void send_sample_to_dac(void);
void write_dac(uint16_t);
void isr_gpio_handler(unsigned int, uint32_t);
void isr_mod_sel_btn(unsigned int, uint32_t);
int64_t alarm_mod_stat_callback(alarm_id_t, void *);
void set_mod_stat_line(bool);
void isr_tempo_sync(unsigned int, uint32_t);
int64_t alarm_mod_beat_callback(alarm_id_t, void *);
bool isr_sample(repeating_timer_t *);
void intermodule_command_handler(void);
void execute_intermodule_command(ctlword_t);

#endif

