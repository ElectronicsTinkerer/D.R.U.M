# from machine import Pin, Timer
# import busio
# from adafruit_neotrellis.neotrellis import NeoTrellis

########################################
# PINS
########################################

# PINS
GPIO_TP9          = 0
GPIO_ENC00        = 1
GPIO_ENC01        = 2
GPIO_TP10         = 3
GPIO_I2C0_SDA0    = 4
GPIO_I2C0_SCL0    = 5
GPIO_ENC10        = 6
GPIO_ENC11        = 7
# GPIO 8
# GPIO 9
GPIO_ENC20        = 10
GPIO_ENC21        = 11
GPIO_I2C0_SDA1    = 12
GPIO_I2C0_SCL1    = 13
GPIO_ENC30        = 14
GPIO_ENC31        = 15
GPIO_CBUS_EXTRA   = 16
GPIO_CBUS_DRDY    = 17
GPIO_CBUS_SCK     = 18
GPIO_CBUS_SDIN    = 19
GPIO_CBUS_SDOUT   = 20
# GPIO 21
GPIO_CBUS_NXTPR = 22
GPIO_BTN0       = 23
GPIO_BTN1       = 24
# GPIO 25
GPIO_ADC0       = 26
GPIO_ADC1       = 27
GPIO_ADC2       = 28
GPIO_ADC3       = 29


# Named pin mapping
TEMPO_ENC0      = GPIO_ENC00
TEMPO_ENC1      = GPIO_ENC01
TSIG_ENC0       = GPIO_ENC10
TSIG_ENC1       = GPIO_ENC11
PLYSTP_BTN      = GPIO_BTN0
MOD_STAT_IRQ    = GPIO_CBUS_EXTRA

# Tempo settings
BPM_MIN =     20
BPM_DEFAULT = 120
BPM_MAX =     600

# Time signature settings
TS_DEFAULT = 1

TIME_SIGNATURES = [
    [3, 4],
    [4, 4],
    [7, 8]
]


# Defaults
bpm = 120
is_running = False
is_mod_selected = False
time_sig = TS_DEFAULT
current_beat = 0
P_DRDY = Pin(GPIO_CBUS_DRDY, Pin.OUT)

# Callback structs
tick_timer = Timer()


def update_screen():
    pass

def update_buttons():
    pass

def isr_timer(timer):
    global P_DRDY
    P_DRDY.toggle()

def isr_module_status():
    pass

def isr_tempo_encoder():
    pass

def isr_play_pause():
    pass

def isr_time_sig_encoder():
    pass

def isr_pad_event():
    pass

def oled_init():
    pass

def pad_init():
    pass

# Init
oled_init();
pad_init();


# Set up the timer for micro beat ticks
tick_timer.init(freq=bpm/60, mode=Timer.PERIODIC, callback=isr_timer)


