/**
 * Wire.h replacement
 * 
 * Zach Baldwin
 */

#include "pico/stdlib.h"

#ifndef _WIRE_H
#define _WIRE_H

#define I2C_BAUDRATE (100 * 1000)
#define BUFFER_LENGTH 128
#define I2C_BUFFER_LENGTH BUFFER_LENGTH

class TwoWire
{
public:
    TwoWire();
    void begin(void);
    void setClock(uint32_t);
    void end();
    void beginTransmission(uint8_t);
    uint8_t endTransmission(void);
    uint8_t endTransmission(uint8_t);
    size_t write(uint8_t);
    size_t write(const uint8_t*, size_t);
    uint8_t requestFrom(uint8_t, size_t, uint8_t);
    int read(void);
    int peek(void);
    void flush(void);
    
private:
    static uint8_t _i2c_channel;
    static uint32_t _i2c_clock;
    static uint8_t _i2c_transmitting;
    static uint8_t _i2c_tx_mode;
    
    static uint8_t _i2c_tx_buffer[];
    static size_t _i2c_tx_addr;
    static size_t _i2c_tx_buf_len;
    static size_t _i2c_tx_buf_index;

    static uint8_t _i2c_rx_buffer[];
    static size_t _i2c_rx_buf_len;
    static size_t _i2c_rx_buf_index;
};

extern TwoWire Wire;

#endif

