# Tamaputer - Tamagotchi Emulator for M5Stack Cardputer

A Tamagotchi emulator for the M5Stack Cardputer using the tamalib library.

## Features

- Full Tamagotchi P1 emulation using tamalib
- **ROM loading from SD card** - No recompiling needed!
- Optimized display rendering on the Cardputer's 240x135 LCD
- Keyboard input mapping for the three Tamagotchi buttons
- Icon display for game status

## Hardware Requirements

- M5Stack Cardputer (ESP32-S3 based)
- MicroSD card (FAT32 formatted)
- USB-C cable for programming

## Controls

The Cardputer keyboard is mapped to the three Tamagotchi buttons:

- **Left Button**: `CTRL` key
- **Middle Button**: `OPT` key
- **Right Button**: `ALT` key
- **Pause Button**: `P` key
- **Save Button**: `Z` key
- **Help Button**: `ESC` key

# Setup Instructions

## M5 Launcher (Easy)

- Download the latest `tamaputer.bin` from the releases page and place it on your SD card.
- Place the ROM file (`tama.b`) in the `tamaputer` directory on your SD Card. Create this directory if it does not exist.
- Start your Cardputer and use M5 Launcher to start `tamamputer.bin`

## Build & Upload

### 1. Install PlatformIO

If you haven't already, install PlatformIO IDE or PlatformIO Core CLI.

### 2. Clone and Build

```bash
cd tamaputer
git submodule update --init --recursive
pio pkg install  # Install dependencies
pio run          # Build the project
```

### 3. Prepare ROM File

**IMPORTANT**: For legal reasons, you must provide your own Tamagotchi ROM dump.

1. Obtain a legal ROM dump from your own device or a legal source
2. The ROM should be exactly **40960 bytes** (40KB) for the original Tamagotchi P1
3. Copy the ROM file to your SD card root directory as `/tamaputer/tama.b`

### 4. Upload to Device

```bash
pio run -t upload
```


## Troubleshooting

If `intelhex` not found:

Install it into the PIO python virtualenv:

```
source ~/.platformio/penv/bin/activate
(penv) $ pip install intelhex
```

## Credits

- **tamalib**: https://github.com/jcrona/tamalib by Jean-Christophe Rona
- **ArduinoGotchi**: https://github.com/GaryZ88/ArduinoGotchi by GaryZ88
- **M5Stack**: For the Cardputer hardware and libraries
- **Original Tamagotchi**: Bandai


## Resources

- [tamalib GitHub](https://github.com/jcrona/tamalib)
- [ArduinoGotchi Github](https://github.com/GaryZ88/ArduinoGotchi)
- [M5Stack Cardputer](https://shop.m5stack.com/products/m5stack-cardputer)
- [PlatformIO Documentation](https://docs.platformio.org/)
