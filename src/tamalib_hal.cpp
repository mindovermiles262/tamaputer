#include "tamalib_hal.h"
#include <stdarg.h>
#include <M5Cardputer.h>

// LCD matrix buffer
static bool_t lcd_matrix[LCD_HEIGHT][LCD_WIDTH];

// LCD icons buffer
static bool_t lcd_icons[ICON_COUNT];

// Button states
static bool_t button_left = 0;
static bool_t button_middle = 0;
static bool_t button_right = 0;

// Display needs update flag
static bool display_dirty = false;

void initDisplay() {
    USBSerial.println("initDisplay() called - clearing screen");
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(1);

    // Clear the LCD matrix
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            lcd_matrix[y][x] = 0;
        }
    }

    // Clear icons
    for (int i = 0; i < ICON_COUNT; i++) {
        lcd_icons[i] = 0;
    }

    // Force an initial display update to show the background
    display_dirty = true;
    updateDisplay();
    USBSerial.println("initDisplay() complete");
}

static unsigned long display_update_count = 0;
void updateDisplay() {
    display_update_count++;

    if (display_update_count % 100 == 0) {
        USBSerial.print("updateDisplay #");
        USBSerial.print(display_update_count);
        USBSerial.print(" dirty=");
        USBSerial.println(display_dirty);
    }

    if (!display_dirty) return;

    if (display_update_count % 100 == 0) {
        USBSerial.println("Drawing display...");
    }

    // Calculate offset to center the display
    int offsetX = (M5Cardputer.Display.width() - (LCD_WIDTH * PIXEL_SIZE)) / 2;
    int offsetY = 20; // Leave space at top for icons

    // Draw LCD matrix
    int pixels_on = 0;
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            if (lcd_matrix[y][x]) pixels_on++;
            uint16_t color = lcd_matrix[y][x] ? TFT_BLACK : TFT_LIGHTGREY;
            M5Cardputer.Display.fillRect(
                offsetX + x * PIXEL_SIZE,
                offsetY + y * PIXEL_SIZE,
                PIXEL_SIZE - 1,
                PIXEL_SIZE - 1,
                color
            );
        }
    }

    if (display_update_count % 100 == 0 || pixels_on > 0) {
        USBSerial.print("Pixels on: ");
        USBSerial.println(pixels_on);
    }

    // Draw icons at the top
    const char* icon_names[] = {"FOOD", "GAME", "LIGHT", "DUCK", "MAIL", "CALL", "ATT", "DISC"};
    int iconX = 10;
    for (int i = 0; i < ICON_COUNT; i++) {
        M5Cardputer.Display.setTextColor(lcd_icons[i] ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
        M5Cardputer.Display.setCursor(iconX, 5);
        M5Cardputer.Display.print(icon_names[i]);
        iconX += 35;
    }

    display_dirty = false;
}

void handleInput() {
    M5Cardputer.update();

    // Map keyboard keys to Tamagotchi buttons
    // Left button (A key)
    bool prev_left = button_left;
    bool prev_middle = button_middle;
    bool prev_right = button_right;

    button_left = M5Cardputer.Keyboard.isKeyPressed('a');
    button_middle = M5Cardputer.Keyboard.isKeyPressed('s');
    button_right = M5Cardputer.Keyboard.isKeyPressed('d');

    // Debug: print button state changes
    if (button_left && !prev_left) USBSerial.println("LEFT button pressed");
    if (button_middle && !prev_middle) USBSerial.println("MIDDLE button pressed");
    if (button_right && !prev_right) USBSerial.println("RIGHT button pressed");
}

// HAL callback: Called when a pixel needs to be set/cleared
static unsigned long pixel_set_count = 0;
void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val) {
    if (x < LCD_WIDTH && y < LCD_HEIGHT) {
        lcd_matrix[y][x] = val;
        display_dirty = true;
        pixel_set_count++;

        // Debug: print periodic pixel stats
        if (pixel_set_count % 50 == 0) {
            USBSerial.print("Pixel updates: ");
            USBSerial.println(pixel_set_count);
        }
    }
}

// HAL callback: Called when an icon needs to be set/cleared
void hal_set_lcd_icon(u8_t icon, bool_t val) {
    if (icon < ICON_COUNT) {
        lcd_icons[icon] = val;
        display_dirty = true;
    }
}

// HAL callback: Called to set CPU frequency (not used on ESP32)
void hal_set_frequency(u32_t freq) {
    // Not implemented for ESP32
}

// HAL callback: Called to enable/disable buzzer
void hal_play_frequency(bool_t en) {
    // Not implemented for now
}

// HAL callback: Called when CPU halts
void hal_halt(void) {
    // Handle halt condition
    M5Cardputer.Display.fillScreen(TFT_RED);
    M5Cardputer.Display.setCursor(50, 60);
    M5Cardputer.Display.println("CPU HALTED");
    while(1) delay(1000);
}

// HAL callback: Called to update the screen
void hal_update_screen(void) {
    updateDisplay();
}

// HAL callback: Get current timestamp in 1/32768 second units
timestamp_t hal_get_timestamp(void) {
    // Convert microseconds to 1/32768 second units
    // 1/32768 second = ~30.5 microseconds
    return (timestamp_t)(micros() / 30.5);
}

// HAL callback: Sleep until timestamp
void hal_sleep_until(timestamp_t ts) {
    timestamp_t now = hal_get_timestamp();
    if (ts > now) {
        // Convert timestamp units (1/32768 s) to microseconds
        unsigned long delay_us = (ts - now) * 30.5;
        delayMicroseconds(delay_us);
    }
}

// HAL callback: Check if logging is enabled for a level
bool_t hal_is_log_enabled(log_level_t level) {
    // Enable interrupt and error logging to see if timers are firing
    return (level == LOG_ERROR || level == LOG_INT) ? 1 : 0;
}

// HAL callback: Log messages
void hal_log(log_level_t level, char *buff, ...) {
    // Print to serial for debugging
    char log_buffer[256];
    va_list args;
    va_start(args, buff);
    vsnprintf(log_buffer, sizeof(log_buffer), buff, args);
    va_end(args);
    USBSerial.print("[TAMALIB] ");
    USBSerial.print(log_buffer);
}

// HAL callback: Handle button input
int hal_handler(void) {
    handleInput();

    // Set button states using proper enum values
    tamalib_set_button(BTN_LEFT, button_left ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_MIDDLE, button_middle ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_RIGHT, button_right ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);

    return 1; // Continue running
}
