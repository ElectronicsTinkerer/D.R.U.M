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
    size_t buf_len
    )
{
    // io_rw_32 *txfifo = (io_rw_32 *) pio->txf[sm];
    // io_rw_32 *rxfifo = (io_rw_32 *) pio->rxf[sm];
    size_t i = buf_len;

    // Reset all input or output data
    pio_sm_clear_fifos(tx_pio, tx_sm);
    // pio_sm_clear_fifos(rx_pio, rx_sm);

    // Send data
    while (i > 0) {
        pio_sm_put_blocking(tx_pio, tx_sm, *tx_buf++);
        --i;
    }

    // Get received data
    // while (i < buf_len) {
        // rx_buf[i++] = pio_sm_get_blocking(rx_pio, rx_sm);
    // }
}


