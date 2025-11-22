/**
 * @file main.cpp
 * @brief Tamagotchi Emulator for M5Stack Cardputer using tamalib
 * @version 0.1
 **/

#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "tamalib_hal.h"

// SD Card pins for M5Cardputer
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// ROM buffer - will be loaded from SD card
// Supported sizes: 12288 (12KB), 40960 (40KB), or other variants
size_t rom_size = 0;
u12_t *g_program = NULL;
bool rom_loaded = false;

// Frame timing - Tamagotchi runs at ~3 FPS
static unsigned long last_screen_update = 0;
static const unsigned long screen_update_interval = 333; // ~3 FPS

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
    .handler = hal_handler
};

bool loadRomFromSD() {
    const char* romFiles[] = {"/tama.b", "/rom.bin"};

    M5Cardputer.Display.println("Checking SD card...");

    // Initialize SPI for SD card
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    // Initialize SD card with M5Cardputer pins
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        M5Cardputer.Display.println("SD card init failed!");
        M5Cardputer.Display.println("Check SD card is");
        M5Cardputer.Display.println("inserted properly");
        return false;
    }

    M5Cardputer.Display.println("SD card OK");

    // Try each ROM filename
    for (int i = 0; i < 2; i++) {
        M5Cardputer.Display.print("Looking for ");
        M5Cardputer.Display.println(romFiles[i]);

        if (SD.exists(romFiles[i])) {
            File romFile = SD.open(romFiles[i], FILE_READ);
            if (!romFile) {
                M5Cardputer.Display.println("Failed to open file");
                continue;
            }

            size_t file_size = romFile.size();
            M5Cardputer.Display.print("Size: ");
            M5Cardputer.Display.print(file_size);
            M5Cardputer.Display.println(" bytes");

            // Tamagotchi ROM: 12288 bytes = 8192 12-bit words packed as 3 bytes per 2 words
            // Each pair of 12-bit words is packed into 3 bytes
            size_t num_words = (file_size * 2) / 3;

            // Allocate memory for unpacked ROM (as u12_t = u16)
            g_program = (u12_t*)malloc(num_words * sizeof(u12_t));
            if (!g_program) {
                M5Cardputer.Display.println("Memory alloc failed!");
                romFile.close();
                return false;
            }

            // Read and unpack ROM file
            // Format: 3 bytes contain 2 x 12-bit words
            // Byte 0: bits 7-0 of word 0
            // Byte 1: bits 3-0 = bits 11-8 of word 0, bits 7-4 = bits 3-0 of word 1
            // Byte 2: bits 7-0 = bits 11-4 of word 1
            uint8_t buffer[3];
            size_t word_index = 0;

            while (romFile.available() >= 3 && word_index < num_words - 1) {
                romFile.read(buffer, 3);

                // First 12-bit word
                g_program[word_index++] = ((buffer[1] & 0x0F) << 8) | buffer[0];

                // Second 12-bit word
                g_program[word_index++] = (buffer[2] << 4) | ((buffer[1] & 0xF0) >> 4);
            }

            romFile.close();
            rom_size = num_words * sizeof(u12_t);

            USBSerial.print("Unpacked ");
            USBSerial.print(word_index);
            USBSerial.println(" words");
            USBSerial.print("First words: 0x");
            USBSerial.print(g_program[0], HEX);
            USBSerial.print(" 0x");
            USBSerial.print(g_program[1], HEX);
            USBSerial.print(" 0x");
            USBSerial.println(g_program[2], HEX);

            M5Cardputer.Display.setTextColor(TFT_GREEN);
            M5Cardputer.Display.println("ROM loaded!");
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            delay(1500);
            return true;
        }
    }

    M5Cardputer.Display.setTextColor(TFT_RED);
    M5Cardputer.Display.println("No ROM found!");
    M5Cardputer.Display.setTextColor(TFT_YELLOW);
    M5Cardputer.Display.println("Place tama.b or");
    M5Cardputer.Display.println("rom.bin on SD card");
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    return false;
}

void setup() {
    // Initialize USB Serial for debugging FIRST (ESP32-S3 uses USBSerial)
    USBSerial.begin(115200);
    delay(500);
    USBSerial.println("\n\n=== TAMAPUTER STARTING ===");
    USBSerial.println("USB Serial initialized");

    // Initialize M5Cardputer
    USBSerial.println("About to call M5Cardputer.begin()");
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    USBSerial.println("M5Cardputer.begin() completed");

    // Initialize display
    initDisplay();

    // Show splash screen
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.println("TAMAPUTER");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(5, 80);
    M5Cardputer.Display.println("Tamagotchi Emulator");
    delay(1000);

    // Load ROM from SD card
    USBSerial.println("Loading ROM from SD card...");
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(2);
    rom_loaded = loadRomFromSD();
    USBSerial.print("ROM loaded: ");
    USBSerial.println(rom_loaded ? "YES" : "NO");

    if (!rom_loaded) {
        // Error already displayed by loadRomFromSD
        delay(10000);  // Show error for 10 seconds
        // Stay in error state
        return;
    }

    // Initialize tamalib with the HAL
    USBSerial.println("Registering HAL...");
    tamalib_register_hal(&hal);

    // Note: tamalib_init returns 0 on SUCCESS (Unix convention)
    // Tamagotchi runs at 32768 Hz, so timestamps should be in units of ~30.5 us
    // Use 32768 for timestamp frequency (timestamps in 1/32768 second units)
    USBSerial.println("Calling tamalib_init...");
    int init_result = tamalib_init(g_program, NULL, 32768);
    USBSerial.print("tamalib_init returned: ");
    USBSerial.println(init_result);

    if (init_result != 0) {
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("Tamalib init failed!");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        rom_loaded = false;
        delay(5000);
        return;
    }

    USBSerial.println("Tamalib init SUCCESS!");

    USBSerial.println("Displaying ready screen...");
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 10);
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setTextColor(TFT_GREEN);
    M5Cardputer.Display.println(" Ready!");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println("Controls:");
    M5Cardputer.Display.println("  A: Left button");
    M5Cardputer.Display.println("  S: Middle button");
    M5Cardputer.Display.println("  D: Right button");
    delay(3000);

    // Clear display for emulator
    USBSerial.println("Setup complete, entering main loop...");
    initDisplay();
}

static unsigned long loop_count = 0;

void loop() {
    loop_count++;

    if (loop_count % 1000 == 0) {
        USBSerial.print("Loop #");
        USBSerial.println(loop_count);
    }

    // Only run emulator if ROM was loaded successfully
    if (rom_loaded) {
        // Step the emulator multiple times per frame
        // Tamagotchi runs at 32768 Hz, so step many times
        for (int i = 0; i < 100; i++) {
            tamalib_step();
        }

        // Call handler for button input (required when using tamalib_step directly)
        hal_handler();

        // Update screen at controlled frame rate (~3 FPS)
        unsigned long now = millis();
        if (now - last_screen_update >= screen_update_interval) {
            last_screen_update = now;
            hal_update_screen();
        }

        // Feed watchdog and yield to system tasks
        // yield();
    } else {
        // ROM not loaded, just idle
        delay(100);
    }

    // Handle M5 system updates
    M5Cardputer.update();
}
