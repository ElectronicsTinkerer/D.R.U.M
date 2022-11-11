/**
 * OLED control functions
 * 
 * Header created from Raspberry Pi (Trading) Ltd.'s 
 * oled example code
 *
 */

#ifndef DRUM_OLED_H
#define DRUM_OLED_H

// commands (see datasheet)
#define OLED_SET_CONTRAST _u(0x81)
#define OLED_SET_ENTIRE_ON _u(0xA4)
#define OLED_SET_NORM_INV _u(0xA6)
#define OLED_SET_DISP _u(0xAE)
#define OLED_SET_MEM_ADDR _u(0x20)
#define OLED_SET_COL_ADDR _u(0x21)
#define OLED_SET_PAGE_ADDR _u(0x22)
#define OLED_SET_DISP_START_LINE _u(0x40)
#define OLED_SET_SEG_REMAP _u(0xA0)
#define OLED_SET_MUX_RATIO _u(0xA8)
#define OLED_SET_COM_OUT_DIR _u(0xC0)
#define OLED_SET_DISP_OFFSET _u(0xD3)
#define OLED_SET_COM_PIN_CFG _u(0xDA)
#define OLED_SET_DISP_CLK_DIV _u(0xD5)
#define OLED_SET_PRECHARGE _u(0xD9)
#define OLED_SET_VCOM_DESEL _u(0xDB)
#define OLED_SET_CHARGE_PUMP _u(0x8D)
#define OLED_SET_HORIZ_SCROLL _u(0x26)
#define OLED_SET_SCROLL _u(0x2E)

#define OLED_ADDR _u(0x3C)
#define OLED_HEIGHT _u(64)
#define OLED_WIDTH _u(128)
#define OLED_PAGE_HEIGHT _u(8)
#define OLED_NUM_PAGES OLED_HEIGHT / OLED_PAGE_HEIGHT
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)

#define OLED_WRITE_MODE _u(0xFE)
#define OLED_READ_MODE _u(0xFF)

extern struct render_area render_area_t;

void fill(uint8_t buf[], uint8_t fill);
void fill_page(uint8_t *buf, uint8_t fill, uint8_t page);
void print_buf_page(uint8_t buf[], uint8_t page);
void print_buf_pages(uint8_t buf[]);
void print_buf_area(uint8_t *buf, render_area_t *area);
void calc_render_area_buflen(render_area_t *area);

#ifdef i2c_default
void oled_send_cmd(uint8_t cmd);
void oled_send_buf(uint8_t buf[], int buflen);
void oled_init();
void render(uint8_t *buf, render_area_t *area);
#endif

#endif

