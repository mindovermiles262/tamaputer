/*
    This is the translation layer between the tamalib functions and how the cardputer
    executes instructions
*/

#include "tamalib_cardputer_hal.h"
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "bitmaps.h"

// SD Card SPI pins for M5Stack Cardputer
#define SD_SPI_SCK_PIN 40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN 12

#define ROM_SIZE 12288 // Expected size of tama.b (12KB)

// External variables from main.cpp
extern String ROM_STATE;
extern String ROM_FILE;

// LCD matrix buffer
static bool_t lcd_matrix[LCD_HEIGHT][LCD_WIDTH];
static bool_t icon_buffer[ICON_NUM] = {0};
static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH / 8] = {{0}};

// LCD icons buffer
static bool_t lcd_icons[ICON_COUNT];

// Button states
static bool_t button_left = 0;
static bool_t button_middle = 0;
static bool_t button_right = 0;
static bool_t button_save = 0;
static bool_t button_pause = 0;
static bool_t button_help = 0;

// static cpu_state_t cpuState;

void draw_triangle(uint16_t x, uint16_t y)
{
    // Draw a simple downward pointing triangle for icon selection
    M5Cardputer.Display.fillTriangle(x + 3, y, x, y + 3, x + 6, y + 3, TFT_WHITE);
}

void draw_icon_bitmap(uint16_t x, uint16_t y, const uint8_t *bitmap)
{
    // Draw 16x9 icon bitmap
    for (int by = 0; by < 9; by++)
    {
        for (int bx = 0; bx < 16; bx++)
        {
            uint8_t byte_idx = (by * 2) + (bx / 8);
            uint8_t bit_idx = 7 - (bx % 8);
            if (bitmap[byte_idx] & (1 << bit_idx))
            {
                M5Cardputer.Display.fillRect(x + bx * 2, y + by * 2, 2, 2, TFT_WHITE);
            }
        }
    }
}

void update_display()
{
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    // Calculate centering offsets
    uint16_t displayStartX = 20;
    uint16_t displayStartY = 10;

    // Draw the Tamagotchi LCD matrix (32x16 pixels, scaled up)
    for (uint8_t y = 0; y < LCD_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < LCD_WIDTH; x++)
        {
            if (lcd_matrix[y][x])
            {
                // Draw pixel scaled up by TAMA_PIXEL_SIZE
                M5Cardputer.Display.fillRect(
                    displayStartX + x * TAMA_PIXEL_SIZE,
                    displayStartY + y * TAMA_PIXEL_SIZE,
                    TAMA_PIXEL_SIZE,
                    TAMA_PIXEL_SIZE,
                    TFT_WHITE);
            }
        }
    }

    // Draw icon selection row at the bottom
    uint16_t iconY = displayStartY + (LCD_HEIGHT * TAMA_PIXEL_SIZE) + 10;
    for (uint8_t i = 0; i < ICON_NUM; i++)
    {
        uint16_t iconX = displayStartX + i * 28;

        // Draw selection triangle if icon is selected
        if (lcd_icons[i])
        {
            draw_triangle(iconX + 6, iconY);
        }

        // Draw the icon bitmap
        draw_icon_bitmap(iconX, iconY + 8, bitmaps + i * 18);
    }
}

void save_state()
{
    M5Cardputer.Display.fillScreen(TFT_DARKGREEN);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(5, 60);
    M5Cardputer.Display.println("Saving state...");

    // Get current CPU state
    state_t *state = tamalib_get_state();
    if (!state)
    {
        M5Cardputer.Display.println("Error: No state!");
        delay(1500);
        return;
    }

    // Initialize SD card
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000))
    {
        M5Cardputer.Display.println("SD init failed!");
        delay(1500);
        return;
    }

    // Delete old state file if it exists
    if (SD.exists(ROM_STATE.c_str()))
    {
        SD.remove(ROM_STATE.c_str());
    }

    // Open file for writing
    File stateFile = SD.open(ROM_STATE.c_str(), FILE_WRITE);
    if (!stateFile)
    {
        M5Cardputer.Display.println("File open failed!");
        SD.end();
        delay(1500);
        return;
    }

    // Write state to file
    // Write CPU registers
    stateFile.write((uint8_t *)state->pc, sizeof(u13_t));
    stateFile.write((uint8_t *)state->x, sizeof(u12_t));
    stateFile.write((uint8_t *)state->y, sizeof(u12_t));
    stateFile.write((uint8_t *)state->a, sizeof(u4_t));
    stateFile.write((uint8_t *)state->b, sizeof(u4_t));
    stateFile.write((uint8_t *)state->np, sizeof(u5_t));
    stateFile.write((uint8_t *)state->sp, sizeof(u8_t));
    stateFile.write((uint8_t *)state->flags, sizeof(u4_t));

    // Write timers
    stateFile.write((uint8_t *)state->tick_counter, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_2hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_4hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_8hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_16hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_32hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_64hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_128hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->clk_timer_256hz_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->prog_timer_timestamp, sizeof(u32_t));
    stateFile.write((uint8_t *)state->prog_timer_enabled, sizeof(bool_t));
    stateFile.write((uint8_t *)state->prog_timer_data, sizeof(u8_t));
    stateFile.write((uint8_t *)state->prog_timer_rld, sizeof(u8_t));

    stateFile.write((uint8_t *)state->call_depth, sizeof(u32_t));

    // Write interrupts (array of INT_SLOT_NUM interrupts)
    for (int i = 0; i < INT_SLOT_NUM; i++)
    {
        stateFile.write((uint8_t *)&state->interrupts[i].factor_flag_reg, sizeof(u4_t));
        stateFile.write((uint8_t *)&state->interrupts[i].mask_reg, sizeof(u4_t));
        stateFile.write((uint8_t *)&state->interrupts[i].triggered, sizeof(bool_t));
        stateFile.write((uint8_t *)&state->interrupts[i].vector, sizeof(u8_t));
    }

    stateFile.write((uint8_t *)state->cpu_halted, sizeof(bool_t));

    // Write memory (most important!)
    stateFile.write((uint8_t *)state->memory, MEM_BUFFER_SIZE);

    stateFile.close();
    SD.end();

    M5Cardputer.Display.println("Saved successfully!");
    USBSerial.println("[*] State saved to SD card");
    delay(1500);
}

// Convert ROM from 4-byte format to 3-byte packed format
// Input: rom_data (12288 bytes), Output: packed_rom (9216 bytes)
void convert_rom_to_12bit(const uint8_t *rom_data, uint8_t *packed_rom)
{
    for (int i = 0; i < (ROM_SIZE / 4); i++)
    {
        uint8_t v1 = rom_data[i * 4];
        uint8_t v2 = rom_data[i * 4 + 1];
        uint8_t v3 = rom_data[i * 4 + 2];
        uint8_t v4 = rom_data[i * 4 + 3];

        packed_rom[i * 3] = (v1 << 4) | ((v2 >> 4) & 0xF);
        packed_rom[i * 3 + 1] = ((v2 & 0xF) << 4) | v3;
        packed_rom[i * 3 + 2] = v4;
    }
}

// Unpack 3-byte packed format to u12_t array
void unpack_rom_to_12bit_array(const uint8_t *packed_rom, int packed_size, u12_t *rom_data)
{
    int rom_word_count = (packed_size * 2) / 3;
    for (int i = 0; i < rom_word_count; i += 2)
    {
        int byte_idx = (i * 3) / 2;
        uint8_t b0 = packed_rom[byte_idx];
        uint8_t b1 = packed_rom[byte_idx + 1];
        uint8_t b2 = packed_rom[byte_idx + 2];

        rom_data[i] = (b0 << 4) | ((b1 >> 4) & 0x0F);
        rom_data[i + 1] = ((b1 & 0x0F) << 8) | b2;
    }
}

u12_t *load_rom()
{
    // Initialize SD card with correct SPI pins
    USBSerial.print("[*] Initializing SD and Loading rom ... ");
    uint8_t *raw_rom = (uint8_t *)malloc(ROM_SIZE);
    bool rom_loaded = false;

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (SD.begin(SD_SPI_CS_PIN, SPI, 25000000))
    {
        File romFile = SD.open(ROM_FILE.c_str(), FILE_READ);
        if (romFile)
        {
            size_t size = romFile.size();
            if (size == ROM_SIZE)
            {
                romFile.read(raw_rom, ROM_SIZE);
                rom_loaded = true;
            }
            romFile.close();
        }
        SD.end();
    }
    USBSerial.println("Done");

    if (!rom_loaded)
    {
        M5Cardputer.Display.fillScreen(TFT_RED);
        M5Cardputer.Display.setCursor(10, 50);
        M5Cardputer.Display.println("ERROR:");
        M5Cardputer.Display.setCursor(10, 70);
        M5Cardputer.Display.printf("%s not found!\n", ROM_FILE.c_str());
        free(raw_rom);
        while (1)
            delay(1000);
    }

    // Convert ROM from 4-byte to 3-byte packed format
    const int packed_size = (ROM_SIZE / 4) * 3; // 9216 bytes
    uint8_t *packed_rom = (uint8_t *)malloc(packed_size);
    convert_rom_to_12bit(raw_rom, packed_rom);
    free(raw_rom); // Don't need the raw ROM anymore

    // Unpack to u12_t array for tamalib
    const int rom_word_count = (packed_size * 2) / 3;
    u12_t *rom_data = (u12_t *)malloc(sizeof(u12_t) * rom_word_count);
    unpack_rom_to_12bit_array(packed_rom, packed_size, rom_data);
    free(packed_rom); // Don't need packed ROM anymore
    USBSerial.println("Done.");
    return rom_data;
}

void pause_game()
{
    static uint8_t volume = 32; // Store volume between pauses

    // Wait for pause key release
    while (M5Cardputer.Keyboard.isPressed())
    {
        M5Cardputer.update();
        delay(10);
    }

    // Function to draw pause screen
    auto draw_pause_screen = []()
    {
        M5Cardputer.Display.fillRect(20, 50, 200, 50, TFT_BLACK);
        M5Cardputer.Display.drawRect(20, 50, 200, 50, TFT_WHITE);

        M5Cardputer.Display.setTextColor(TFT_YELLOW);
        M5Cardputer.Display.setCursor(90, 60);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.println("PAUSED");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(45, 80);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.println("UP/DOWN : Vol  ESC : Help");
    };

    // Draw initial pause screen
    draw_pause_screen();

    bool unpaused = false;
    while (!unpaused)
    {
        M5Cardputer.update();

        // Handle input
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
        {
            bool volume_changed = false;

            if (M5Cardputer.Keyboard.isKeyPressed(';')) // Up arrow (semicolon key)
            {
                // Increase volume
                if (volume <= 245)
                    volume += 10;
                else
                    volume = 255;
                M5Cardputer.Speaker.setVolume(volume);
                M5Cardputer.Speaker.tone(2000, 50); // Play test beep
                volume_changed = true;
            }
            else if (M5Cardputer.Keyboard.isKeyPressed('.')) // Down arrow (period key)
            {
                // Decrease volume
                if (volume >= 10)
                    volume -= 10;
                else
                    volume = 0;
                M5Cardputer.Speaker.setVolume(volume);
                if (volume > 0)
                    M5Cardputer.Speaker.tone(2000, 50); // Play test beep
                volume_changed = true;
            }
            else
            {
                // Any other key unpauses
                unpaused = true;
            }

            // Redraw if volume changed
            if (volume_changed)
            {
                draw_pause_screen();
            }

            // Wait for key release
            while (M5Cardputer.Keyboard.isPressed())
            {
                M5Cardputer.update();
                delay(10);
            }
        }

        delay(10);
    }

    update_display();
}

void draw_help_screen()
{
    M5Cardputer.Display.setCursor(0, 10);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_GREEN);
    M5Cardputer.Display.println("Controls");
    M5Cardputer.Display.println("  A : CTRL");
    M5Cardputer.Display.println("  B : OPT");
    M5Cardputer.Display.println("  C : ALT");

    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.println("Menu");
    M5Cardputer.Display.println("  P : Pause");
    M5Cardputer.Display.println("  Z : Save");
}

void display_help()
{
    draw_help_screen();
    while (M5Cardputer.Keyboard.isPressed())
    {
        M5Cardputer.update();
        delay(10);
    }

    while (!M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        M5Cardputer.update();
        delay(100);
    }
}

bool_t load_from_state()
{
    USBSerial.println("[*] Checking for saved state...");

    // Initialize SD card
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000))
    {
        USBSerial.println("[!] SD init failed for state load");
        return 0;
    }

    // Check if state file exists
    if (!SD.exists(ROM_STATE.c_str()))
    {
        USBSerial.println("[*] No saved state found, starting fresh");
        SD.end();
        return 0;
    }

    // Open file for reading
    File stateFile = SD.open(ROM_STATE.c_str(), FILE_READ);
    if (!stateFile)
    {
        USBSerial.println("[!] State file open failed!");
        SD.end();
        return 0;
    }

    // Get current CPU state (we'll write into these pointers)
    state_t *state = tamalib_get_state();
    if (!state)
    {
        USBSerial.println("[!] Error: No state!");
        stateFile.close();
        SD.end();
        return 0;
    }

    // Read state from file
    // Read CPU registers
    stateFile.read((uint8_t *)state->pc, sizeof(u13_t));
    stateFile.read((uint8_t *)state->x, sizeof(u12_t));
    stateFile.read((uint8_t *)state->y, sizeof(u12_t));
    stateFile.read((uint8_t *)state->a, sizeof(u4_t));
    stateFile.read((uint8_t *)state->b, sizeof(u4_t));
    stateFile.read((uint8_t *)state->np, sizeof(u5_t));
    stateFile.read((uint8_t *)state->sp, sizeof(u8_t));
    stateFile.read((uint8_t *)state->flags, sizeof(u4_t));

    // Read timers
    stateFile.read((uint8_t *)state->tick_counter, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_2hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_4hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_8hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_16hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_32hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_64hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_128hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->clk_timer_256hz_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->prog_timer_timestamp, sizeof(u32_t));
    stateFile.read((uint8_t *)state->prog_timer_enabled, sizeof(bool_t));
    stateFile.read((uint8_t *)state->prog_timer_data, sizeof(u8_t));
    stateFile.read((uint8_t *)state->prog_timer_rld, sizeof(u8_t));

    stateFile.read((uint8_t *)state->call_depth, sizeof(u32_t));

    // Read interrupts (array of INT_SLOT_NUM interrupts)
    for (int i = 0; i < INT_SLOT_NUM; i++)
    {
        stateFile.read((uint8_t *)&state->interrupts[i].factor_flag_reg, sizeof(u4_t));
        stateFile.read((uint8_t *)&state->interrupts[i].mask_reg, sizeof(u4_t));
        stateFile.read((uint8_t *)&state->interrupts[i].triggered, sizeof(bool_t));
        stateFile.read((uint8_t *)&state->interrupts[i].vector, sizeof(u8_t));
    }

    stateFile.read((uint8_t *)state->cpu_halted, sizeof(bool_t));

    // Read memory (most important!)
    stateFile.read((uint8_t *)state->memory, MEM_BUFFER_SIZE);

    stateFile.close();
    SD.end();

    // Refresh hardware state from loaded memory
    tamalib_refresh_hw();

    USBSerial.println("[*] State loaded successfully!");
    USBSerial.printf("[*] PC: %d, A: %d, B: %d\n", *state->pc, *state->a, *state->b);
    return 1;
}

void handle_input()
{
    M5Cardputer.update();

    // Map keyboard keys to Tamagotchi buttons
    button_left = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_LEFT);
    button_middle =
        M5Cardputer.Keyboard.isKeyPressed(M5_BTN_CENTER) ||
        M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
        M5Cardputer.Keyboard.isKeyPressed(' ');
    button_right = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_RIGHT);
    button_save = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_SAVE);
    button_pause = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_PAUSE);
    button_help = M5Cardputer.Keyboard.isKeyPressed(M5_BTN_HELP);
}

// HAL callback: Called when a pixel needs to be set/cleared
void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    if (x < LCD_WIDTH && y < LCD_HEIGHT)
    {
        lcd_matrix[y][x] = val;
    }
}

// HAL callback: Called when an icon needs to be set/cleared
void hal_set_lcd_icon(u8_t icon, bool_t val)
{
    if (icon < ICON_COUNT)
    {
        lcd_icons[icon] = val;
    }
}

// HAL callback: Called to set CPU frequency (not used on ESP32)
void hal_set_frequency(u32_t freq)
{
    // Not implemented
}

// HAL callback: Called to enable/disable buzzer
void hal_play_frequency(bool_t en)
{
    if (en)
    {
        // Play a beep tone at 4000 Hz
        M5Cardputer.Speaker.tone(2000, 50);
    }
    else
    {
        // Stop the tone
        M5Cardputer.Speaker.stop();
    }
}

void hal_halt(void)
{
    M5Cardputer.Display.fillScreen(TFT_RED);
    M5Cardputer.Display.setCursor(50, 60);
    M5Cardputer.Display.println("CPU HALTED");
    while (1)
        delay(1000);
}

// HAL callback: Called to update the screen
void hal_update_screen(void)
{
    update_display();
}

// HAL callback: Get current timestamp in 1/32768 second units
timestamp_t hal_get_timestamp(void)
{
    return millis() * 1000;
}

// HAL callback: Sleep until timestamp
void hal_sleep_until(timestamp_t ts)
{
    // Not Implemented
}

// HAL callback: Check if logging is enabled for a level
bool_t hal_is_log_enabled(log_level_t level)
{
    return (level == LOG_ERROR || level == LOG_INT) ? 1 : 0;
}

// HAL callback: Log messages
void hal_log(log_level_t level, char *buff, ...)
{
    // Only log if enabled for this level
    if (!hal_is_log_enabled(level))
    {
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
int hal_handler(void)
{
    static bool prev_save = 0;
    static bool prev_pause = 0;
    static bool prev_help = 0;

    handle_input();

    // Save state on rising edge (button press, not hold)
    if (button_save && !prev_save)
    {
        save_state();
    }
    prev_save = button_save;

    // Pause on rising edge (button press, not hold)
    if (button_pause && !prev_pause)
    {
        pause_game();
    }
    prev_pause = button_pause;

    if (button_help && !prev_help)
    {
        display_help();
    }

    // Set button states using proper enum values
    tamalib_set_button(BTN_LEFT, button_left ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_MIDDLE, button_middle ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    tamalib_set_button(BTN_RIGHT, button_right ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
    return 1; // Continue running
}
