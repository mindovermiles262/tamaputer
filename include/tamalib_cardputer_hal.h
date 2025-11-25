#ifndef TAMALIB_HAL_H
#define TAMALIB_HAL_H

#include <M5Cardputer.h>

extern "C" {
    #include <tamalib.h>
}

// Tamagotchi LCD dimensions
#define LCD_WIDTH 32
#define LCD_HEIGHT 16

// Icon positions
#define ICON_COUNT 8

// Scale factor for display
#define PIXEL_SIZE 6

// HAL callback functions required by tamalib
extern "C" {
    int hal_handler(void);
    void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val);
    void hal_set_lcd_icon(u8_t icon, bool_t val);
    void hal_set_frequency(u32_t freq);
    void hal_play_frequency(bool_t en);
    void hal_update_screen(void);
    timestamp_t hal_get_timestamp(void);
    void hal_sleep_until(timestamp_t ts);
    void hal_halt(void);
    bool_t hal_is_log_enabled(log_level_t level);
    void hal_log(log_level_t level, char *buff, ...);
}

// Helper functions
void initDisplay();
void updateDisplay();
void handleInput();

#endif // TAMALIB_HAL_H
