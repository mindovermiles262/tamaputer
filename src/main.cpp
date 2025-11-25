/**
 * @file main.cpp
 * @brief Tamagotchi Emulator for M5Stack Cardputer using tamalib
 * @version 0.1
 **/

#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

#include "rom_12bit.h"

extern "C" {
    #include "tamalib.h"
}

#include "tamalib_cardputer_hal.h"

// static exec_mode_t exec_mode = EXEC_MODE_RUN;
// static u32_t step_depth = 0;
static timestamp_t screen_ts = 0;
static u32_t ts_freq;

// HAL functions structure for tamalib (must match order in hal.h)
static hal_t hal = {
  .malloc = malloc,
  .free = free,
  .halt = hal_halt,
  .is_log_enabled = hal_is_log_enabled,
  .log = hal_log,
  .sleep_until = hal_sleep_until,
  .get_timestamp = hal_get_timestamp,
  .update_screen = hal_update_screen,
  .set_lcd_matrix = hal_set_lcd_matrix,
  .set_lcd_icon = hal_set_lcd_icon,
  .set_frequency = hal_set_frequency,
  .play_frequency = hal_play_frequency,
  .handler = hal_handler,
};

void setup() {
    // Initialize USB Serial for debugging FIRST (ESP32-S3 uses USBSerial)
    USBSerial.begin(115200);
    delay(500);
    USBSerial.println("\n\n=== TAMAPUTER STARTING ===");
    // Initialize M5Cardputer
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    // Show splash screen
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setTextColor(TFT_GREEN);
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.println("TAMAPUTER");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(5, 80);
    M5Cardputer.Display.println("Tamagotchi Emulator");
    delay(2000);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Load Toma ROM
    USBSerial.print("[*] Loading Tomagotchi ROM ... ");
    const int rom_byte_size = sizeof(g_program_b12);
    const int rom_word_count = (rom_byte_size * 2) / 3;
    u12_t *rom_data = (u12_t *)malloc(sizeof(u12_t) * rom_word_count);
    // Convert packed 12-bit data from PROGMEM to u12_t array
    for (int i = 0; i < rom_word_count; i += 2) {
        int byte_idx = (i * 3) / 2;
        uint8_t b0 = pgm_read_byte(&g_program_b12[byte_idx]);
        uint8_t b1 = pgm_read_byte(&g_program_b12[byte_idx + 1]);
        uint8_t b2 = pgm_read_byte(&g_program_b12[byte_idx + 2]);

        rom_data[i] = (b0 << 4) | ((b1 >> 4) & 0x0F);
        rom_data[i + 1] = ((b1 & 0x0F) << 8) | b2;
    }
    USBSerial.println("Done.");

    // Initialize TamaLib
    USBSerial.print("[*] Initializing Tamalib ... ");
    // g_hal = &hal;  // Set global HAL pointer
    tamalib_register_hal(&hal);
    tamalib_set_framerate(10);  // Set framerate to 10 FPS like arduinogotchi
    ts_freq = 1000000;  // 1MHz
    tamalib_init(rom_data, NULL, ts_freq);
    USBSerial.println("Done.");
}

void loop() {
    timestamp_t ts;
    g_hal->handler();
    tamalib_step();
    ts = g_hal->get_timestamp();
    if (ts - screen_ts >= ts_freq / 10) {  // Only update at ~10 FPS
        screen_ts = ts;
        g_hal->update_screen();
    }
}
