/**
 * Wire.h replacement
 * 
 * Pico I2C docs:
 * https://github.com/raspberrypi/pico-sdk/blob/426e46126b5a1efaea4544cdb71ab81b61983034/src/rp2_common/hardware_i2c/include/hardware/i2c.h
 * 
 * Arduino Wire.cpp:
 * https://github.com/esp8266/Arduino/blob/master/libraries/Wire/Wire.cpp
 * 
 * Arduino Wire.h Reference:
 * https://www.arduino.cc/reference/en/language/functions/communication/wire/begin/
 * 
 * Zach Baldwin
 */

extern "C" {
    #include <stdlib.h>
    #include <string.h>
    #include <inttypes.h>
    #include "hardware/i2c.h"
    #include "pico/stdlib.h"
}

#include "Wire.h"

uint8_t TwoWire::_i2c_channel = 0;
uint32_t TwoWire::_i2c_clock = I2C_BAUDRATE;

uint8_t TwoWire::_i2c_rx_buffer[I2C_BUFFER_LENGTH];
size_t TwoWire::_i2c_rx_buf_len = 0;
size_t TwoWire::_i2c_rx_buf_index = 0;

uint8_t TwoWire::_i2c_tx_buffer[I2C_BUFFER_LENGTH];
size_t TwoWire::_i2c_tx_buf_len = 0;
size_t TwoWire::_i2c_tx_buf_index = 0;
uint8_t TwoWire::_i2c_tx_mode = 1; // Arduino levels are inverted pico levels
size_t TwoWire::_i2c_tx_addr = 0;

uint8_t TwoWire::_i2c_transmitting = 0;

TwoWire::TwoWire()
{
    /* empty */
}

void TwoWire::begin(void)
{
    if (_i2c_channel == 0) {
        i2c_init(i2c0, _i2c_clock);
    }
    else {
        i2c_init(i2c1, _i2c_clock);
    }
}

void TwoWire::end()
{
    if (_i2c_channel == 0) {
        i2c_deinit(i2c0);
    }
    else {
        i2c_deinit(i2c1);
    }
}

// rate: in hertz
void TwoWire::setClock(uint32_t rate)
{
    _i2c_clock = rate;
    
    if (_i2c_channel == 0) {
        i2c_set_baudrate(i2c0, rate);
    }
    else {
        i2c_set_baudrate(i2c1, rate);
    }
}

void TwoWire::beginTransmission(uint8_t addr)
{
    _i2c_transmitting = 1;
    _i2c_tx_addr = addr;
    _i2c_tx_buf_len = 0;
    _i2c_tx_buf_index = 0;
}

uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(1);
}

uint8_t TwoWire::endTransmission(uint8_t tx_mode)
{
    if (_i2c_tx_buf_len > I2C_BUFFER_LENGTH) {
        
        _i2c_tx_buf_len = 0;
        _i2c_tx_buf_index = 0;
        _i2c_transmitting = 0;
    
        return 1; // Data too long
    }
    
    int ret = 0;
    if (_i2c_tx_buf_len != 0) {
        if (_i2c_channel == 0) {
            ret = i2c_write_blocking(
                i2c0,
                _i2c_tx_addr,
                _i2c_tx_buffer,
                _i2c_tx_buf_len,
                !tx_mode
                );
        }
        else {
            ret = i2c_write_blocking(
                i2c1,
                _i2c_tx_addr,
                _i2c_tx_buffer,
                _i2c_tx_buf_len,
                !tx_mode
                );
        }
    }
    
    _i2c_tx_buf_len = 0;
    _i2c_tx_buf_index = 0;
    _i2c_transmitting = 0;
    
    return (ret == PICO_ERROR_GENERIC) ? 5 : 0;
}

size_t TwoWire::write(uint8_t data)
{
    if (_i2c_transmitting) {
        if (_i2c_tx_buf_len >= I2C_BUFFER_LENGTH) {
            return 0;
        }
        _i2c_tx_buffer[_i2c_tx_buf_index] = data;
        ++_i2c_tx_buf_index;
        _i2c_tx_buf_len = _i2c_tx_buf_index;
    }
    else {
        if (_i2c_channel == 0) {
            i2c_write_raw_blocking(
                i2c0,
                &data,
                1
                );
        }
        else {
            i2c_write_raw_blocking(
                i2c1,
                &data,
                1
                );
        }
    }
    return 1;
}

size_t TwoWire::write(const uint8_t *buf, size_t len)
{
    if (_i2c_transmitting) {
        for (size_t i = 0; i < len; ++i) {
            if (!write(buf[i])) {
                return i;
            }
        }
    }
    else {
        if (_i2c_channel == 0) {
            i2c_write_raw_blocking(
                i2c0,
                buf,
                len
                );
        }
        else {
            i2c_write_raw_blocking(
                i2c1,
                buf,
                len
                );
        }
    }
    return len;
}



uint8_t TwoWire::requestFrom(uint8_t addr, size_t len, uint8_t stop)
{
    if (len > I2C_BUFFER_LENGTH) {
        len = I2C_BUFFER_LENGTH;
    }
    
    size_t read;
    if (_i2c_channel == 0) {
        read = i2c_read_timeout_us(
            i2c0,
            addr,
            _i2c_rx_buffer,
            len,
            !stop,
            I2C_REQUEST_TIMEOUT_US
            );
    }
    else {
        read = i2c_read_timeout_us(
            i2c1,
            addr,
            _i2c_rx_buffer,
            len,
            !stop,
            I2C_REQUEST_TIMEOUT_US
            );
    }

    if (read == PICO_ERROR_GENERIC) {
        read = 0;
    }
    _i2c_rx_buf_index = 0;
    _i2c_rx_buf_len = read;
    return read;
}

int TwoWire::read(void)
{
    int value = -1;
    if (_i2c_rx_buf_index < _i2c_rx_buf_len) {
        value = _i2c_rx_buffer[_i2c_rx_buf_index];
        ++_i2c_rx_buf_index;
    }
    return value;
}
        
int TwoWire::peek(void)
{
    int value = -1;
    if (_i2c_rx_buf_index < _i2c_rx_buf_len) {
        value = _i2c_rx_buffer[_i2c_rx_buf_index];
    }
    return value;
}

void TwoWire::flush(void)
{
    _i2c_rx_buf_len = 0;
    _i2c_rx_buf_index = 0;
    _i2c_tx_buf_len = 0;
    _i2c_tx_buf_index = 0;
}


TwoWire Wire;

