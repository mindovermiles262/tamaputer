/**
 * @file main.cpp
 * @brief Tamagotchi Emulator for M5Stack Cardputer using tamalib
 * @version 0.1
 **/

#include <M5Cardputer.h>

extern "C"
{
#include "tamalib.h"
}

#include "tamalib_cardputer_hal.h"

// Global ROM and state file paths
String ROM_FILE = "/tamaputer/tama.b";
String ROM_STATE = "/tamaputer/tama.state";

static timestamp_t screen_ts = 0;
static u32_t ts_freq;
hal_t *g_hal;

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

void setup()
{
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
    M5Cardputer.Display.setCursor(40, 10);
    M5Cardputer.Display.println("TAMAPUTER");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(5, 30);
    M5Cardputer.Display.println("Tamagotchi Emulator");

    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(2);

    M5Cardputer.Display.setCursor(5, 80);
    M5Cardputer.Display.println("Hold");
    M5Cardputer.Display.setCursor(5, 100);
    M5Cardputer.Display.println("  SPACE: New Game");
    delay(2500);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Show ROM file selection menu
    M5Cardputer.update();
    u12_t *rom_data = load_rom();
    bool keyStartNewGame =
        M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
        M5Cardputer.Keyboard.isKeyPressed(' ');

    // Initialize TamaLib
    USBSerial.print("[*] Initializing Tamalib ... ");
    g_hal = &hal;
    tamalib_register_hal(&hal);
    tamalib_set_framerate(10); // Set framerate to 10 FPS
    ts_freq = 1000000;         // 1MHz
    tamalib_init(rom_data, NULL, ts_freq);
    USBSerial.println("Done.");

    // Load saved state if it exists
    if (!keyStartNewGame)
    {
        load_from_state();
    }
}

void loop()
{
    timestamp_t ts;
    g_hal->handler();
    tamalib_step();
    ts = g_hal->get_timestamp();
    if (ts - screen_ts >= ts_freq / 10)
    {
        screen_ts = ts;
        g_hal->update_screen();
    }
}
