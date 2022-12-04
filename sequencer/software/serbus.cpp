/**
 * 32-bit SPI-like intermodule communication bus
 * 
 * Authors:
 * Zach Baldwin
 */


#include "sequencer.pio.h"
#include "serbus.h"


/**
 * Flush the RX and TX FIFOs on the sequencer
 * This function BLOCKS
 * 
 * @param *rx_buf[] 
 * @param *tx_buf[]
 * @param buf_len The number of words to copy into rx buf and out of tx buf
 * @return 
 */
void intermodule_serbus_txrx(
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
    // io_rw_32 *txfifo = (io_rw_32 *) pio->txf[sm];
    // io_rw_32 *rxfifo = (io_rw_32 *) pio->rxf[sm];
    size_t i = buf_len;

    // Reset all input or output data
    pio_sm_clear_fifos(tx_pio, tx_sm);
    pio_sm_clear_fifos(rx_pio, rx_sm);

    // Send data
    gpio_put(drdy_pin, 0); // Signal start of data
    while (i > 0) {
        pio_sm_put_blocking(tx_pio, tx_sm, *tx_buf++);
        --i;
    }

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
    gpio_put(drdy_pin, 1); // End of data
    gpio_put(clk_pin, 0); // Reset clock to advance state machines on the modules
}


