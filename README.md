# Tamaputer - Tamagotchi Emulator for M5Stack Cardputer

A Tamagotchi emulator for the M5Stack Cardputer using the tamalib library.

## Features

- Full Tamagotchi P1 emulation using tamalib
- **ROM loading from SD card** - No recompiling needed!
- Optimized display rendering on the Cardputer's 240x135 LCD
- Keyboard input mapping for the three Tamagotchi buttons
- Icon display for game status
- Automatic ROM detection (tries `tama.b` and `rom.bin`)

## Hardware Requirements

- M5Stack Cardputer (ESP32-S3 based)
- MicroSD card (FAT32 formatted)
- USB-C cable for programming

## Controls

The Cardputer keyboard is mapped to the three Tamagotchi buttons:

- **Left Button**: `A` key
- **Middle Button**: `S` key
- **Right Button**: `D` key

## Setup Instructions

### 1. Install PlatformIO

If you haven't already, install PlatformIO IDE or PlatformIO Core CLI.

### 2. Clone and Build

```bash
cd tamaputer
pio lib install  # Install dependencies
pio run          # Build the project
```

### 3. Prepare ROM File

**IMPORTANT**: For legal reasons, you must provide your own Tamagotchi ROM dump.

1. Obtain a legal ROM dump from your own device or a legal source
2. The ROM should be exactly **40960 bytes** (40KB) for the original Tamagotchi P1
3. Copy the ROM file to your SD card root directory as either:
   - `tama.b` (preferred)
   - `rom.bin` (alternative)

The emulator will automatically try both filenames on startup.

### 4. Upload to Device

```bash
pio run -t upload
```

If `intelhex` not found:

Install it into the PIO python virtualenv:

```
source ~/.platformio/penv/bin/activate
(penv) $ pip install intelhex
```

## Display Layout

```
+----------------------------------+
| FOOD GAME LIGHT DUCK MAIL ... |  <- Icons
+----------------------------------+
|                                  |
|    +--------------------+        |
|    |                    |        |
|    |  Tamagotchi LCD    |        |
|    |   (32x16 pixels)   |        |
|    |                    |        |
|    +--------------------+        |
|                                  |
+----------------------------------+
```

## Technical Details

### Display

- Native Tamagotchi resolution: 32x16 pixels
- Scaled up by 6x for the Cardputer display
- Icons displayed as text at the top
- Pixels are inverted (black on light grey) to match original LCD appearance

### Performance

- Emulation runs at approximately 1MHz (configurable)
- Display updates only when pixels change (dirty flag optimization)
- Input polling every frame

### Memory

- ROM: 40KB (loaded from SD card into RAM)
- Emulator state: ~2KB RAM
- Display buffer: 512 bytes (32x16 bits)
- Total RAM usage: ~43KB

## Troubleshooting

### SD Card Issues

**"SD card init failed!"**
- Ensure the SD card is properly inserted
- Try formatting the SD card as FAT32
- Test the SD card in another device first
- Some SD cards may not be compatible - try a different brand

**"No ROM found!"**
- Verify the ROM file is named exactly `tama.b` or `rom.bin`
- Check the ROM is in the root directory (not in a folder)
- Confirm the file is exactly 40960 bytes
- Try copying the file again to the SD card

**"Wrong size!"**
- Your ROM file is not 40960 bytes
- Re-dump your ROM or verify the source
- Check the file wasn't corrupted during transfer

### Build Errors

If you get tamalib include errors:
```bash
git submodule update --init --recursive  # Initialize tamalib submodule
pio lib install  # Re-install dependencies
rm -rf .pio      # Clean build
pio run          # Rebuild
```

### Display Issues

- If display is garbled, check that the ROM loaded successfully
- Try reducing PIXEL_SIZE in `tamalib_hal.h`
- Check that M5Cardputer libraries are up to date
- The emulator may take a moment to initialize after loading

### No Response to Buttons

- Ensure the ROM loaded successfully (check startup messages)
- Verify M5Cardputer.update() is called in loop
- Check keyboard mapping in `tamalib_hal.cpp`

## Advanced Features

### Save States

To add save/load functionality, implement SPIFFS or LittleFS storage:

```cpp
// Save state
void saveState() {
    state_t* state = tamalib_get_state();
    File f = SPIFFS.open("/tamagotchi.sav", "w");
    f.write((uint8_t*)state, sizeof(state_t));
    f.close();
}

// Load state
void loadState() {
    if (SPIFFS.exists("/tamagotchi.sav")) {
        File f = SPIFFS.open("/tamagotchi.sav", "r");
        state_t state;
        f.readBytes((char*)&state, sizeof(state_t));
        f.close();
        tamalib_set_state(&state);
    }
}
```

### Sound

Add a buzzer to a GPIO pin and implement the sound callback in the HAL.

## Credits

- **tamalib**: https://github.com/jcrona/tamalib by Jean-Christophe Rona
- **M5Stack**: For the Cardputer hardware and libraries
- **Original Tamagotchi**: Bandai

## License

This project is for educational purposes only. Tamagotchi is a trademark of Bandai.
You must own a legal copy of the Tamagotchi ROM to use this emulator.

## Resources

- [tamalib GitHub](https://github.com/jcrona/tamalib)
- [M5Stack Cardputer](https://shop.m5stack.com/products/m5stack-cardputer)
- [PlatformIO Documentation](https://docs.platformio.org/)
