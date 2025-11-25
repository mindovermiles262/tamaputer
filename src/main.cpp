/**
 * @file main.cpp
 * @brief Tamagotchi Emulator for M5Stack Cardputer using tamalib
 * @version 0.1
 **/

#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>

extern "C" {
    #include "tamalib.h"
}

#include "tamalib_cardputer_hal.h"

#define ROM_FILE "/tama.b"
#define ROM_SIZE 12288  // Expected size of tama.b (12KB)

// SD Card SPI pins for M5Stack Cardputer
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

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

// Convert ROM from 4-byte format to 3-byte packed format (like TamaRomConvert.java)
// Input: rom_data (12288 bytes), Output: packed_rom (9216 bytes)
void convertRomTo12Bit(const uint8_t* rom_data, uint8_t* packed_rom) {
    for (int i = 0; i < (ROM_SIZE / 4); i++) {
        uint8_t v1 = rom_data[i * 4];
        uint8_t v2 = rom_data[i * 4 + 1];
        uint8_t v3 = rom_data[i * 4 + 2];
        uint8_t v4 = rom_data[i * 4 + 3];

        packed_rom[i * 3]     = (v1 << 4) | ((v2 >> 4) & 0xF);
        packed_rom[i * 3 + 1] = ((v2 & 0xF) << 4) | v3;
        packed_rom[i * 3 + 2] = v4;
    }
}

// Unpack 3-byte packed format to u12_t array
void unpackRomTo12BitArray(const uint8_t* packed_rom, int packed_size, u12_t* rom_data) {
    int rom_word_count = (packed_size * 2) / 3;
    for (int i = 0; i < rom_word_count; i += 2) {
        int byte_idx = (i * 3) / 2;
        uint8_t b0 = packed_rom[byte_idx];
        uint8_t b1 = packed_rom[byte_idx + 1];
        uint8_t b2 = packed_rom[byte_idx + 2];

        rom_data[i]     = (b0 << 4) | ((b1 >> 4) & 0x0F);
        rom_data[i + 1] = ((b1 & 0x0F) << 8) | b2;
    }
}

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


    // Initialize SD card with correct SPI pins
    USBSerial.print("[*] Initializing SD and Loading rom ... ");
    uint8_t* raw_rom = (uint8_t*)malloc(ROM_SIZE);
    bool rom_loaded = false;

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        File romFile = SD.open(ROM_FILE, FILE_READ);
        if (romFile) {
            size_t size = romFile.size();
            if (size == ROM_SIZE) {
                romFile.read(raw_rom, ROM_SIZE);
                rom_loaded = true;            }
            romFile.close();
        }
        SD.end();
    }
    USBSerial.println("Done");

    if (!rom_loaded) {
        M5Cardputer.Display.fillScreen(TFT_RED);
        M5Cardputer.Display.setCursor(10, 50);
        M5Cardputer.Display.println("ERROR:");
        M5Cardputer.Display.setCursor(10, 70);
        M5Cardputer.Display.printf("%s not found!\n", ROM_FILE);
        while(1) delay(1000);
    }

    // Convert ROM from 4-byte to 3-byte packed format
    const int packed_size = (ROM_SIZE / 4) * 3;  // 9216 bytes
    uint8_t* packed_rom = (uint8_t*)malloc(packed_size);
    convertRomTo12Bit(raw_rom, packed_rom);
    free(raw_rom);  // Don't need the raw ROM anymore

    // Unpack to u12_t array for tamalib
    const int rom_word_count = (packed_size * 2) / 3;
    u12_t* rom_data = (u12_t*)malloc(sizeof(u12_t) * rom_word_count);
    unpackRomTo12BitArray(packed_rom, packed_size, rom_data);
    free(packed_rom);  // Don't need packed ROM anymore
    USBSerial.println("Done.");

    // Initialize TamaLib
    USBSerial.print("[*] Initializing Tamalib ... ");
    g_hal = &hal;
    tamalib_register_hal(&hal);
    tamalib_set_framerate(10);  // Set framerate to 10 FPS
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
