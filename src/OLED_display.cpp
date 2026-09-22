#include "OLED_display.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Types.hpp"
#include "driver/i2c_master.h"
#include "esp_log.h"

esp_err_t OLED_display::oled_send_command(uint8_t com) {
    uint8_t buffer[2];
    buffer[0] = 0x00;
    buffer[1] = com;
    return i2c_master_transmit(handle, buffer, sizeof(buffer), 1000);
}
void OLED_display::oled_init() {
    oled_send_command(0xAE);  // Вимкнути екран
    oled_send_command(0x8D);  // Готуємося ввімкнути помпу
    oled_send_command(0x14);  // Вмикаємо її
    oled_send_command(0xAF);  // Вмикаємо екран
    //  oled_send_command(0xA1);  // Дзеркально розвертаю по горизонталі
    // oled_send_command(0xC8);  // по вертикалі поворот
}
void OLED_display::oled_clear_buffer() { memset(&framebuffer, 0, sizeof(framebuffer)); }

OLED_display::OLED_display() = default;
void OLED_display::oled_update() {
    for (uint8_t page = 0; page < 8; page++) {
        uint8_t temp_buf[129];
        temp_buf[0] = 0x40;

        memcpy(temp_buf + 1, &framebuffer[page * 128], 128);

        oled_send_command(0xB0 + page);
        oled_send_command(0x00);
        oled_send_command(0x10);

        i2c_master_transmit(handle, temp_buf, 129, 1000);
    }
}
void OLED_display::draw_pixel(int x, int y, bool color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int index = (y / 8) * 128 + x;
    int bit = y % 8;

    if (color) {
        framebuffer[index] |= (1 << bit);
    } else {
        framebuffer[index] &= ~(1 << bit);
    }
}
void OLED_display::draw_char(int x, int y, char c, bool color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int f_ind = c - ' ';
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            bool pixel_on = ((font5x7[f_ind][col] >> row) & 1);
            if (!color) {
                pixel_on = !pixel_on;
            }
            draw_pixel(x + col, y + row, pixel_on);
        }
    }
}
void OLED_display::draw_str(int x, int y, const char* str, bool color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    while (*str) {
        draw_char(x, y, *str, color);
        x += 6;
        ++str;
    }
}
void OLED_display::draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color) {
    if (x1 < 0 || x1 >= 128 || y1 < 0 || y1 >= 64 || x2 < 0 || x2 >= 128 || y2 < 0 || y2 >= 64) return;

    uint16_t dx = abs(x1 - x2);
    uint16_t dy = abs(y1 - y2);
    int8_t sx = x1 < x2 ? 1 : -1;
    int8_t sy = y1 < y2 ? 1 : -1;

    int16_t err = dx - dy;

    while (true) {
        int16_t err2 = 2 * err;

        draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;

        if (err2 > dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}
void OLED_display::fill_rect(uint8_t x1, uint8_t y1, uint8_t w, uint8_t h, bool color) {
    for (uint8_t x = x1; x < x1 + w; x++) {
        for (uint8_t y = y1; y < y1 + h; y++) {
            draw_pixel(x, y, color);
        }
    }
}
void OLED_display::draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color) {
    draw_line(x1, y1, x2, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x2, y1, color);
    draw_line(x2, y2, x1, y2, color);
}
void OLED_display::begin(i2c_master_dev_handle_t handle_) {
    handle = handle_;
    oled_init();
    ESP_LOGI(TAG, "OLED_display successfuly launched!");
}