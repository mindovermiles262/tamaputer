#ifndef TAMALIB_HAL_H
#define TAMALIB_HAL_H

#include <M5Cardputer.h>

extern "C" {
    #include <tamalib.h>
}

// Input controls
#define M5_BTN_LEFT KEY_LEFT_CTRL
#define M5_BTN_CENTER KEY_OPT
#define M5_BTN_RIGHT KEY_LEFT_ALT
#define M5_BTN_SAVE 'z'
#define M5_BTN_PAUSE 'p'

// Tamagotchi LCD dimensions
#define LCD_WIDTH 32
#define LCD_HEIGHT 16

// Scale factor for display
#define TAMA_PIXEL_SIZE 5

// Icon positions
#define ICON_COUNT 8

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
void update_display();
void handle_input();
void save_state();
u12_t* load_rom();
bool_t load_from_state();

#endif // TAMALIB_HAL_H
