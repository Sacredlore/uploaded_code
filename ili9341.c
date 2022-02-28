#include "mcu_shell.h"
#include "ili9341.h"

/* array of initialization, bytes sent to ILI9341 */
const uint8_t glcd_bytes[] = {
    ILI9341_CMD_SOFTWARE_RESET, 0x80, // 0x01, __delay_ms(200);
    ILI9341_CMD_POWER_CONTROL_B, 3, 0x00, 0x8B, 0x30, // 0xCF
    ILI9341_CMD_POWER_ON_SEQUENCE_CONTROL, 4, 0x67, 0x03, 0x12, 0x81, // 0xED
    ILI9341_CMD_DRIVER_TIMING_CONTROL_A_E8, 3, 0x85, 0x10, 0x7A, // 0xE8
    ILI9341_CMD_POWER_CONTROL_A, 5, 0x39, 0x2C, 0x00, 0x34, 0x02, // 0xCB
    ILI9341_CMD_PUMP_RATIO_CONTROL, 1, 0x20, // 0xF7
    ILI9341_CMD_DRIVER_TIMING_CONTROL_B, 2, 0x00, 0x00, // 0xEA
    ILI9341_CMD_POWER_CONTROL_1, 1, 0x23, // 0xC0, power control VRH[5:0], ?, 4.60V -> 0x23 to 3.0V -> 0x03
    ILI9341_CMD_POWER_CONTROL_2, 1, 0x10, // 0xC1, power control SAP[2:0];BT[3:0], ?, 0x10 -> to 0x00
    ILI9341_CMD_VCOM_CONTROL_1, 2, 0x3F, 0x3C, // 0xC5, ?, 0x3F - 5.875V, 0x3C - 4,275V, reserved?
    ILI9341_CMD_VCOM_CONTROL_2, 1, 0xB7, // 0xC7 // ?
    ILI9341_CMD_MEMORY_ACCESS_CONTROL, 1, ILI9341_MX | ILI9341_MV | ILI9341_BGR, // ILI9341_MV - landscape or portrait orientation
    ILI9341_CMD_COLMOD_PIXEL_FORMAT_SET, 1, 0x55, // 0x3A, 16-bits pixel
    ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL, 2, 0x00, 0x1B, // 0xB1, fosc, 27 clocks
    ILI9341_CMD_VERT_SCROLL_START_ADDRESS, 1, 0x00, // 0x37, vertical scroll
    ILI9341_CMD_DISPLAY_FUNCTION_CONTROL, 2, 0x0A, 0xA2, // 0xB6
    ILI9341_CMD_ENABLE_3_GAMMA_CONTROL, 1, 0x00, // 0xF2, 3Gamma function disable
    ILI9341_CMD_GAMMA_SET, 1, 0x01, // 0x26, gamma curve
    ILI9341_CMD_POSITIVE_GAMMA_CORRECTION, 15, // 0xE0, set gamma
    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION, 15, // 0xE1, set gamma
    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    ILI9341_CMD_SLEEP_OUT, 0x81, // __delay_ms(120);
    ILI9341_CMD_DISPLAY_ON, 0x81, // __delay_ms(120);
    0x00 // end of list
};

uint8_t display_orientation = ORIENT_LANDSCAPE;
uint16_t display_x_max = 320, display_y_max = 240;

uint8_t glcd_spi_send_8(uint8_t byte)
{
    SSPBUF = byte; // put byte into SPI buffer
    while (!SSPSTATbits.BF)
        NOP(); // wait for the transfer to finish
    return SSPBUF; // save the read value
}

void glcd_spi_send_16(uint16_t word)
{
    uint8_t temp;

    SSPBUF = (word >> 8); // most significant byte
    while (!SSPSTATbits.BF); // wait for the transfer to finish
    temp = SSPBUF;

    SSPBUF = (word & 0xFF); // least significant byte
    while (!SSPSTATbits.BF); // wait for the transfer to finish
    temp = SSPBUF;
}

void glcd_spi_send_16_2(uint8_t word[])
{
    uint8_t c = 2; // sizeof (word) / sizeof (uint8_t)

    while (c--) {
        SSPBUF = *word;
        while (!SSPSTATbits.BF)
            NOP();
        *word++ = SSPBUF;
    };
}

void SPI_TransmitReceive(uint8_t bytes[], uint8_t size)
{
    for (int i = 0; i < size; i++) {
        SSPBUF = bytes[i];
        while (!BF);
        bytes[i] = SSPBUF;
    }
}

void touchscreen_receive_x_y(uint16_t *x, uint16_t *y)
{
    uint32_t average_x = 0;
    uint32_t average_y = 0;

    uint8_t x_raw[2] = {0x00, 0x00};
    uint8_t y_raw[2] = {0x00, 0x00};

    touchscreen_select();
    for (int i = 0; i < 10; i++) {
        glcd_spi_send_8(0xD0); // TOUCHSCREEN_READ_X
        __delay_ms(10);
        SPI_TransmitReceive(x_raw, sizeof (x_raw));
        glcd_spi_send_8(0x90); // TOUCHSCREEN_READ_Y
        __delay_ms(10);
        SPI_TransmitReceive(y_raw, sizeof (y_raw));
        //        average_x += (((uint16_t) x_raw[0]) << 8) | ((uint16_t) x_raw[1]);
        //        average_y += (((uint16_t) y_raw[0]) << 8) | ((uint16_t) y_raw[1]);
        average_x = (((uint16_t) x_raw[0]) << 8) | ((uint16_t) x_raw[1]);
        average_y = (((uint16_t) y_raw[0]) << 8) | ((uint16_t) y_raw[1]);
    }
    touchscreen_unselect();

    *x = (uint16_t) average_x;
    *y = (uint16_t) average_y;
}

void glcd_set_column(uint16_t start_column, uint16_t end_column) // set column address (X axis)
{
    uint8_t s[2];

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_COLUMN_ADDRESS_SET); // 0x2A

    glcd_write_data();
    glcd_spi_send_16(start_column);
    glcd_spi_send_16(end_column);
}

void glcd_set_page(uint16_t start_page, uint16_t end_page) // set page address (Y axis)
{
    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_PAGE_ADDRESS_SET); // 0x2B

    glcd_write_data();
    glcd_spi_send_16(start_page);
    glcd_spi_send_16(end_page);
}

void glcd_set_pixel(uint16_t x0, uint16_t y0, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0);
    glcd_set_page(y0, y0);

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    glcd_spi_send_16(color);
    glcd_unselect();
}

void glcd_set_orientation(uint8_t orientation)
{
    if (orientation == ORIENT_PORTRAIT) {
        display_x_max = 240;
        display_y_max = 320;

        glcd_select();
        glcd_set_column(0, 0);
        glcd_set_page(0, 0);

        glcd_write_command();
        glcd_spi_send_8(ILI9341_CMD_MEMORY_ACCESS_CONTROL); // 0x36

        glcd_write_data();
        glcd_spi_send_8(0x08); //MV bit is not set
        glcd_unselect();
    }

    if (orientation == ORIENT_LANDSCAPE) {
        display_x_max = 320;
        display_y_max = 240;

        glcd_select();
        glcd_set_column(0, 0);
        glcd_set_page(0, 0);

        glcd_write_command();
        glcd_spi_send_8(ILI9341_CMD_MEMORY_ACCESS_CONTROL); // 0x36

        glcd_write_data();
        glcd_spi_send_8(0x28); // MV bit is set
        glcd_unselect();
    }
}

void glcd_draw_hline(uint16_t x0, uint16_t y0, uint16_t length, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0 + length);
    glcd_set_page(y0, y0);

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (int i = 0; i < length; i++) {
        glcd_spi_send_16(color);
    }
    glcd_unselect();
}

void glcd_draw_vline(uint16_t x0, uint16_t y0, uint16_t length, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0); // set column address range
    glcd_set_page(y0, y0 + length); // set page address range

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (int i = 0; i < length; i++) {
        glcd_spi_send_16(color);
    }
    glcd_unselect();
}

void glcd_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    glcd_draw_hline(x0, y0, x1 - x0 + 1, color); // top line
    glcd_draw_vline(x0, y0, y1 - y0 + 1, color); // left line
    glcd_draw_hline(x0, y1, x1 - x0 + 1, color); // bottom line
    glcd_draw_vline(x1, y0, y1 - y0 + 1, color); // right line
}

void glcd_fill_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    do {
        glcd_draw_hline(x0, y0, x1 - x0 + 1, color);
    } while (y1 - y0++);
}

void glcd_fill_screen(uint16_t color) // GLCD_BACKGROUND_COLOR
{
    uint16_t c_loop = display_x_max * (display_y_max / 8);

    glcd_select();
    glcd_set_column(0, display_x_max - 1); // set column address range
    glcd_set_page(0, display_y_max - 1); // set page address range	

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (uint16_t i = 0; i < c_loop; i++) // write 320*240/8 = 9600
    {
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
    }
    glcd_unselect();
}

void glcd_draw_character(uint8_t c, uint16_t x0, uint16_t y0, glcd_font font, uint16_t foreground_color, uint16_t background_color)
{
    uint8_t x, y, w;
    uint8_t offset_x, offset_y;

    offset_x = font.width - 1, offset_y = 0;
    for (y = 0; y < font.height; y++) {
        w = *(font.face + c * font.height + y); // w = font8x14_face[c][y];
        for (x = 0; x < font.width; x++) {
            if (w & 1) { // letter pixels
                glcd_set_pixel(-x + x0 + offset_x, y + y0 + offset_y, foreground_color);
            } else { // letter background
                glcd_set_pixel(-x + x0 + offset_x, y + y0 + offset_y, background_color);
            }
            w = w >> 1;
        }
    }
}

void glcd_draw_string(uint8_t *string, uint16_t x0, uint16_t y0, glcd_font font, uint16_t foreground_color, uint16_t background_color)
{
    uint8_t length = strlen(string);

    while (length--) {
        glcd_draw_character(*string++, x0, y0, font, foreground_color, background_color);
        x0 += font.spacing;
    }
}

void glcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) // Bresenham's line algorithm
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (x0 != x1 || y0 != y1) {
        glcd_set_pixel(x0, y0, color);

        int e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void glcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color) // midpoint circle algorithm
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    glcd_set_pixel(x0, y0 + radius, color);
    glcd_set_pixel(x0, y0 - radius, color);
    glcd_set_pixel(x0 + radius, y0, color);
    glcd_set_pixel(x0 - radius, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        glcd_set_pixel(x0 + x, y0 + y, color);
        glcd_set_pixel(x0 - x, y0 + y, color);
        glcd_set_pixel(x0 + x, y0 - y, color);
        glcd_set_pixel(x0 - x, y0 - y, color);
        glcd_set_pixel(x0 + y, y0 + x, color);
        glcd_set_pixel(x0 - y, y0 + x, color);
        glcd_set_pixel(x0 + y, y0 - x, color);
        glcd_set_pixel(x0 - y, y0 - x, color);
    }
}

void glcd_select_font(uint8_t font_id, glcd_font *font)
{
    const glcd_font fontlist[] = {
        { font8x8_face, 8, 8, 8},
        { font8x12_face, 8, 12, 8},
        { font8x14_face, 8, 14, 8}
    };

    *font = fontlist[font_id];
}

void glcd_draw_framed_string(uint8_t *string, uint16_t x, uint16_t y, glcd_font font)
{
    uint16_t frame_length = strlen(string) * font.width;

    glcd_draw_rectangle(x, y, x + frame_length + 3, y + font.height + 3, GLCD_BORDER_COLOR); // exterior frame
    glcd_draw_rectangle(x + 1, y + 1, x + frame_length + 2, y + font.height + 2, GLCD_COMPONENT_BACKGROUND_COLOR); // interior frame
    glcd_draw_string(string, x + 2, y + 2, font, GLCD_NORMAL_FONT_COLOR, GLCD_COMPONENT_BACKGROUND_COLOR);
}

void glcd_component_event(uint8_t v, char *s)
{

}

void glcd_draw_component(uint8_t *caption, uint16_t x, uint16_t y, uint8_t font_id)
{
    uint8_t frame_margin = 3, text_space;
    glcd_font font;
    glcd_element element;

    glcd_select_font(font_id, &font);
    text_space = strlen(caption) * font.width;

    element.x0 = x;
    element.y0 = y;
    element.x1 = x + text_space + frame_margin;
    element.y1 = y + font.height + frame_margin;

    glcd_draw_rectangle(element.x0, element.y0, element.x1, element.y1, GLCD_BORDER_COLOR); // exterior frame
    glcd_draw_rectangle(element.x0 + 1, element.y0 + 1, element.x1 - 1, element.y1 - 1, GLCD_COMPONENT_BACKGROUND_COLOR); // interior frame
    glcd_draw_string(caption, element.x0 + 2, element.y0 + 2, font, GLCD_NORMAL_FONT_COLOR, GLCD_COMPONENT_BACKGROUND_COLOR);
}

void glcd_draw_component2(uint8_t *caption, uint16_t x, uint16_t y, uint8_t font_id)
{
    uint8_t frame_margin = 3, text_space, value_space;
    glcd_font font;
    glcd_element element;

    glcd_select_font(font_id, &font);
    text_space = strlen(caption) * font.width;
    value_space = 5 * font.width;

    element.x0 = x;
    element.y0 = y;
    element.x1 = x + text_space + value_space + frame_margin;
    element.y1 = y + font.height + frame_margin;

    glcd_draw_rectangle(element.x0, element.y0, element.x1, element.y1, GLCD_BORDER_COLOR); // exterior frame
    glcd_draw_rectangle(element.x0 + 1, element.y0 + 1, element.x1 - 1, element.y1 - 1, GLCD_COMPONENT_BACKGROUND_COLOR); // interior frame
    glcd_draw_string(caption, element.x0 + 2, element.y0 + 2, font, GLCD_NORMAL_FONT_COLOR, GLCD_COMPONENT_BACKGROUND_COLOR);
    glcd_draw_string("0.000", element.x0 + 2 + text_space, element.y0 + 2, font, GLCD_NORMAL_FONT_COLOR, GLCD_COMPONENT_BACKGROUND_COLOR);
}

void glcd_draw_component3(uint8_t *caption, uint16_t x, uint16_t y, uint8_t font_id) // range switcher
{

}

void glcd_clear_framed_string(uint8_t *string, uint16_t x, uint16_t y, glcd_font font)
{
    uint16_t frame_length = strlen(string) * font.width;

    glcd_fill_rectangle(x, y, x + frame_length + 3, y + font.height + 3, GLCD_BACKGROUND_COLOR);
}

void glcd_initialize(void)
{
    glcd_select();
    for (uint8_t *p = (uint8_t *) glcd_bytes; *p != 0x00;) {
        glcd_write_command();
        glcd_spi_send_8(*p++);

        if (*p == 0x80) {
            __delay_ms(200);
            p++;
            continue;
        }
        if (*p == 0x81) {
            __delay_ms(120);
            p++;
            continue;
        }

        glcd_write_data();
        for (uint8_t i = *p++; i > 0; i--) {
            glcd_spi_send_8(*p++);
        }
    };
    glcd_unselect();
}

void glcd_demo(void)
{
    glcd_font font;

    //glcd_set_orientation(ORIENT_PORTRAIT);
    //glcd_set_orientation(ORIENT_LANDSCAPE);
    glcd_fill_screen(GLCD_BACKGROUND_COLOR);
    glcd_select_font(GLCD_FONT_8X8, &font);
    glcd_draw_framed_string("GLCD_DEMO_FUNCTION", 15, 25, font);
    glcd_select_font(GLCD_FONT_8X12, &font);
    glcd_draw_framed_string("DRAW_LINE_AND_CIRCLE", 15, 45, font);
    glcd_select_font(GLCD_FONT_8X14, &font);
    glcd_draw_framed_string("TOUCSCREEN_CONTROLLER", 15, 65, font);

    glcd_draw_line(125, 100, 150, 125, ILI9341_MC_CYAN);
    glcd_draw_line(150, 125, 100, 125, ILI9341_MC_CYAN);
    glcd_draw_line(100, 125, 125, 100, ILI9341_MC_CYAN);

    glcd_draw_rectangle(170, 100, 195, 125, ILI9341_MC_YELLOW);

    glcd_draw_circle(30, 180, 10, ILI9341_MC_RED);
    glcd_draw_circle(80, 180, 20, ILI9341_MC_DARKGREEN);
    glcd_draw_circle(150, 180, 30, ILI9341_MC_BROWN);

    //    glcd_clear_framed_string("ILI9341 ili9341", 25, 25, font8x8);
    //    glcd_clear_framed_string("ILI9341 ili9341", 25, 75, font8x12);
    //    glcd_clear_framed_string("ILI9341 ili9341", 25, 125, font8x14);
}
