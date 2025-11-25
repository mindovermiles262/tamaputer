/*
    This is the translation layer between the tamalib functions and how the cardputer
    executes instructions
*/

#include "tamalib_cardputer_hal.h"
#include <M5Cardputer.h>
#include "bitmaps.h"

// LCD matrix buffer
static bool_t lcd_matrix[LCD_HEIGHT][LCD_WIDTH];
static bool_t icon_buffer[ICON_NUM] = {0};
static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH/8] = {{0}};

// LCD icons buffer
static bool_t lcd_icons[ICON_COUNT];

// Button states
static bool_t button_left = 0;
static bool_t button_middle = 0;
static bool_t button_right = 0;
static bool_t button_save = 0;

// static cpu_state_t cpuState;

void drawTriangle(uint16_t x, uint16_t y) {
  // Draw a simple downward pointing triangle for icon selection
  M5Cardputer.Display.fillTriangle(x+3, y, x, y+3, x+6, y+3, TFT_WHITE);
}

void drawIconBitmap(uint16_t x, uint16_t y, const uint8_t* bitmap) {
  // Draw 16x9 icon bitmap
  for (int by = 0; by < 9; by++) {
    for (int bx = 0; bx < 16; bx++) {
      uint8_t byte_idx = (by * 2) + (bx / 8);
      uint8_t bit_idx = 7 - (bx % 8);
      if (bitmap[byte_idx] & (1 << bit_idx)) {
        M5Cardputer.Display.fillRect(x + bx * 2, y + by * 2, 2, 2, TFT_WHITE);
      }
    }
  }
}

void updateDisplay() {
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  // Calculate centering offsets
  uint16_t displayStartX = 20;
  uint16_t displayStartY = 10;

  // Draw the Tamagotchi LCD matrix (32x16 pixels, scaled up)
  for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
      if (lcd_matrix[y][x]) {
        // Draw pixel scaled up by TAMA_PIXEL_SIZE
        M5Cardputer.Display.fillRect(
          displayStartX + x * TAMA_PIXEL_SIZE,
          displayStartY + y * TAMA_PIXEL_SIZE,
          TAMA_PIXEL_SIZE,
          TAMA_PIXEL_SIZE,
          TFT_WHITE
        );
      }
    }
  }

  // Draw icon selection row at the bottom
  uint16_t iconY = displayStartY + (LCD_HEIGHT * TAMA_PIXEL_SIZE) + 10;
  for (uint8_t i = 0; i < ICON_NUM; i++) {
    uint16_t iconX = displayStartX + i * 28;

    // Draw selection triangle if icon is selected
    if (lcd_icons[i]) {
      drawTriangle(iconX + 6, iconY);
    }

    // Draw the icon bitmap
    drawIconBitmap(iconX, iconY + 8, bitmaps + i * 18);
  }
}

void save_state() {
  // cpu_get_state(&cpuState);
  // Save CPU state as a blob
  // preferences.putBytes("cpuState", &cpuState, sizeof(cpu_state_t));
  // Save memory as a blob
  // preferences.putBytes("memory", cpuState.memory, MEMORY_SIZE);
  // Mark as initialized
  // preferences.putUChar("initialized", 12);
  // preferences.end();
  // Serial.println("Saved");
  // Show save notification on screen
  M5Cardputer.Display.fillScreen(TFT_DARKGREEN);
  M5Cardputer.Display.setCursor(5, 60);
  M5Cardputer.Display.println("Saving ...");
  M5Cardputer.Display.print("\nNot Yet Implemented!");
  delay(1000);
}

void handleInput() {
    M5Cardputer.update();
    // Map keyboard keys to Tamagotchi buttons
    bool prev_left = button_left;
    bool prev_middle = button_middle;
    bool prev_right = button_right;
    bool prev_save = button_save;

    button_left = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_LEFT);
    button_middle = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_CENTER) ||
                    M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)  ||
                    M5Cardputer.Keyboard.isKeyPressed(' ');
    button_right = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_RIGHT);
    button_save = M5Cardputer.Keyboard.isKeyPressed('z');

    // Debug: print button state changes
    if (button_left && !prev_left) USBSerial.println("LEFT button pressed");
    if (button_middle && !prev_middle) USBSerial.println("MIDDLE button pressed");
    if (button_right && !prev_right) USBSerial.println("RIGHT button pressed");
    if (button_save && !prev_save) USBSerial.println("SAVE button pressed");
}

// HAL callback: Called when a pixel needs to be set/cleared
void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val) {
    if (x < LCD_WIDTH && y < LCD_HEIGHT) {
        lcd_matrix[y][x] = val;
    }
}

// HAL callback: Called when an icon needs to be set/cleared
void hal_set_lcd_icon(u8_t icon, bool_t val) {
    if (icon < ICON_COUNT) {
        lcd_icons[icon] = val;
    }
}

// HAL callback: Called to set CPU frequency (not used on ESP32)
void hal_set_frequency(u32_t freq) {
    // Not implemented
}

// HAL callback: Called to enable/disable buzzer
void hal_play_frequency(bool_t en) {
    // Not implemented
}

void hal_halt(void) {
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
    return millis() * 1000;
}

// HAL callback: Sleep until timestamp
void hal_sleep_until(timestamp_t ts) {
  // Not Implemented
}

// HAL callback: Check if logging is enabled for a level
bool_t hal_is_log_enabled(log_level_t level) {
    return (level == LOG_ERROR || level == LOG_INT) ? 1 : 0;
}

// HAL callback: Log messages
void hal_log(log_level_t level, char *buff, ...) {
    // Only log if enabled for this level
    if (!hal_is_log_enabled(level)) {
        return;
    }

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
    if (button_save) {
      save_state();
    };

    // Set button states using proper enum values
    tamalib_set_button(BTN_LEFT, button_left ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_MIDDLE, button_middle ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_RIGHT, button_right ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    return 1; // Continue running
}

