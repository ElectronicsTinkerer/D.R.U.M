/**
 * 32-bit SPI-like intermodule communication bus
 * 
 * Authors:
 * Zach Baldwin
 */

#include "pico/platform.h"

#include "sequencer.pio.h"
#include "serbus.h"


/**
 * Flush the RX and TX FIFOs on the sequencer
 * This function BLOCKS
 * 
 * @param rx_pio 
 * @param rx_sm 
 * @param tx_pio 
 * @param tx_sm 
 * @param *rx_buf 
 * @param *tx_buf 
 * @param buf_len The number of words to copy into rx buf and out of tx buf
 * @param drdy_pin 
 * @param clk_pin 
 * @return 
 */
void __time_critical_func(intermodule_serbus_txrx)(
    const PIO rx_pio,
    const uint rx_sm,
    const PIO tx_pio,
    const uint tx_sm,
    uint32_t *rx_buf,
    uint32_t *tx_buf,
    size_t buf_len,
    uint drdy_pin,
    uint clk_pin
    )
{
    if (buf_len == 0) {
        return;
    }
    
    size_t i = buf_len;

    // Reset all input or output data
    pio_sm_clear_fifos(tx_pio, tx_sm);
    pio_sm_clear_fifos(rx_pio, rx_sm);

    // Send data
    gpio_put(drdy_pin, 0); // Signal start of data
    while (i > 0) {
        pio_interrupt_clear(tx_pio, tx_sm);
        pio_sm_put_blocking(tx_pio, tx_sm, *tx_buf++);
        --i;
    }

    // Wait for transmission to complete.
    // First, we wait for the FIFO to be emptied.
    // Once that is empty, we have to wait for the
    // shifting of the last word to complete. This
    // is accomplished by waiting for IRQ flag 1
    // in the TX state machine. The SM sets it once
    // shifting is complete
    while (!pio_sm_is_tx_fifo_empty(tx_pio, tx_sm)) {
        /* spin */
        pio_interrupt_clear(tx_pio, tx_sm); // Continue advancing the FIFO as needed
    }
    pio_interrupt_clear(tx_pio, tx_sm+1);
    while (!pio_interrupt_get(tx_pio, tx_sm+1)) {
        // Spin, waiting for the completion of shifting        
    }
    gpio_put(drdy_pin, 1); // End of data
    pio_interrupt_clear(tx_pio, tx_sm); // Reset clock to advance state machines on the modules

    // Get received data
    uint32_t v;
    while (i < buf_len) {
        v = pio_sm_get_blocking(rx_pio, rx_sm);

        // Allow null buffers to ignore return data
        if (rx_buf) {
            rx_buf[i] = v;
        }
        ++i;
    }
}


