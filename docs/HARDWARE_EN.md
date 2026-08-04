# NM-TV-154 Hardware Documentation

[English](HARDWARE_EN.md) | [简体中文](HARDWARE.md)

NM-TV-154 is a small smart display (SmallTV) officially released by NMTech, equipped with a 1.54-inch 240×240 color screen. It is both a crypto lottery miner (hashrate up to 1000+ KH/s, about 20× that of NerdMiner) and a general-purpose **ESP32 development board**, supporting users to develop custom firmware.

- Product page: https://www.nmminer.com/product/nm-tv-154/
- Official custom firmware tutorial: https://www.nmminer.com/2026/03/02/how-to-develop-nm-tv-custom-firmware/
- Schematic: see "NM-TV-154 Schematic" in the official tutorial

---

## 1. Hardware Specifications

| Item | Specification |
| --- | --- |
| MCU | ESP32-WROOM-32 (dual-core, 240 MHz), PlatformIO uses `board = esp32dev` |
| Flash | 4 MB |
| RAM | 384KB ROM; 512KB SRAM |
| Wi-Fi | 2.4 GHz 802.11 b/g/n |
| Display | 1.54-inch ST7789 TFT, 240×240 resolution, SPI interface (HSPI) |
| Backlight | GPIO19 PWM dimming, **active low** |
| Input | 1 capacitive touch key (ESP32 touch channel **T9**, i.e. GPIO32) |
| LCD power control | GPIO21 control, **active low** |
| Power / flashing | USB (on-board USB-to-serial, 115200 baud) |

## 2. Pin Definitions

### 2.1 Display (ST7789, SPI)

| Signal | GPIO | Description |
| --- | --- | --- |
| TFT_MOSI | 13 | SPI data output |
| TFT_SCLK | 14 | SPI clock |
| TFT_CS | 15 | Chip select |
| TFT_DC | 2 | Data / command select |
| TFT_RST | -1 | Not connected (software reset) |
| TFT_MISO | -1 | Not connected |
| TFT_BL | 19 | Backlight PWM, active low |
| — | 21 | LCD power rail switch, active low |

Recommended SPI frequencies:

```
SPI_FREQUENCY       40000000
SPI_READ_FREQUENCY  20000000
```

### 2.2 Touch Key

| Item | Value |
| --- | --- |
| Touch channel | T9 (GPIO32) |
| Read method | `touchRead(T9)` |
| Press threshold | raw value < 90 (reference value, calibrate per device) |

### 2.3 Power and Backlight Notes

1. **The LCD power rail must be enabled after power-on**, otherwise the screen will not display:

   ```cpp
   pinMode(21, OUTPUT);
   digitalWrite(21, LOW);   // LOW = power rail enabled
   ```

2. **The backlight is active low**. When using PWM, duty cycle and brightness are inverted (duty 0 = brightest, 1023 = off). Reference implementation (10-bit resolution, 5 kHz):

   ```cpp
   ledcSetup(0, 5000, 10);
   ledcAttachPin(19, 0);
   ledcWrite(0, 0);        // brightest
   ledcWrite(0, 1023);     // off
   ```

3. **Color inversion**: the ST7789 on this board needs `tft.invertDisplay(true)` to show correct colors.

## 3. PlatformIO Configuration Highlights

TFT_eSPI pin definitions are injected through `build_flags` in `platformio.ini` (`-DUSER_SETUP_LOADED=1`), **no need to modify `User_Setup.h` in the library directory**:

```ini
build_flags =
    -DUSER_SETUP_LOADED=1
    -DST7789_DRIVER=1
    -DUSE_HSPI_PORT
    -DTFT_WIDTH=240
    -DTFT_HEIGHT=240
    -DTFT_MOSI=13
    -DTFT_SCLK=14
    -DTFT_CS=15
    -DTFT_DC=2
    -DTFT_RST=-1
    -DTFT_MISO=-1
    -DSPI_FREQUENCY=40000000
    -DSPI_READ_FREQUENCY=20000000
```

> If developing in Arduino IDE, modify `Arduino/libraries/TFT_eSPI/User_Setup.h` according to the official tutorial,
> and additionally define `#define TFT_BL 19` and `#define TFT_BACKLIGHT_ON LOW`.

## 4. Flashing Methods

### 4.1 Serial Flashing (PlatformIO)

```bash
pio run -e nm-tv-154 -t upload --upload-port COMx
```

If the board cannot enter download mode, power-cycle it into flash mode and try again.

### 4.2 Full-Chip Image Flashing (esptool)

This project automatically generates `full_image.bin` (bootloader + partition table + firmware) after compilation:

```bash
esptool.py --chip esp32 write_flash 0x0 .pio/build/nm-tv-154/full_image.bin
```

This image can also be used for factory mass flashing or complete recovery flashing other than the official firmware.

## 5. Reference Projects

- [RockBase-iot/deskbuddy-tv](https://github.com/RockBase-iot/deskbuddy-tv) — a complete application based on the same hardware (weather / emoji / clock / web config / OTA). The HAL layer pin definitions in `src/hal/nm_tv_154_hal.cpp` are consistent with this project and can be used as an advanced reference.
- [NMminer1024 (GitHub)](https://github.com/NMminer1024) — official miner firmware.
