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
#define GPIO_TP9        0
#define GPIO_ENC00      1
#define GPIO_ENC01      2
#define GPIO_TP10       3
#define GPIO_I2C0_SDA0  4
#define GPIO_I2C0_SCL0  5
#define GPIO_ENC10      6
#define GPIO_ENC11      7
// GPIO 8
// GPIO 9
#define GPIO_ENC20      10
#define GPIO_ENC21      11
#define GPIO_I2C0_SDA1  12
#define GPIO_I2C0_SCL1  13
#define GPIO_ENC30      14
#define GPIO_ENC31      15
#define GPIO_CBUS_EXTRA 16
#define GPIO_CBUS_DRDY  17
#define GPIO_CBUS_SCK   18
#define GPIO_CBUS_SDIN  19
#define GPIO_CBUS_SDOUT 20
// GPIO 21
#define GPIO_CBUS_NXTPR 22
#define GPIO_BTN0       23
#define GPIO_BTN1       24
// GPIO 25
#define GPIO_ADC0       26
#define GPIO_ADC1       27
#define GPIO_ADC2       28
#define GPIO_ADC3       29


// Named pin mapping
#define TEMPO_ENC0      GPIO_ENC00
#define TEMPO_ENC1      GPIO_ENC01
#define TSIG_ENC0       GPIO_ENC10
#define TSIG_ENC1       GPIO_ENC11
#define PLYSTP_BTN      GPIO_BTN0
#define MOD_STAT_IRQ    GPIO_CBUS_EXTRA

// Tempo settings
#define BPM_MIN 20
#define BPM_DEFAULT 120
#define BPM_MAX 600
#define TOTAL_UBEATS 9
#define MIN_UBEAT (-(TOTAL_UBEATS-1)/2) // Microbeats before the beat
#define MAX_UBEAT ((TOTAL_UBEATS-1)/2)  // Microbeats after the beat

// Time signature settings
#define TS_DEFAULT 1

unsigned int time_signatures[][2] = {
    {3, 4},
    {4, 4},
    {7, 8}
};


// Function signatures
void update_screen(void);
void update_buttons(void);
bool isr_timer(repeating_timer_t *rt);
void isr_module_status(unsigned int, uint32_t);
void isr_tempo_encoder(unsigned int, uint32_t);
void isr_play_pause(unsigned int, uint32_t);
void isr_time_sig_encoder(unsigned int, uint32_t);
void isr_pad_event(void); // This might not be possible, depending on whether or not the pad can send interrupts

#endif

