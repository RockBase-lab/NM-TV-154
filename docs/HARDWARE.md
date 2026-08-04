# NM-TV-154 硬件说明文档

[简体中文](HARDWARE.md) | [English](HARDWARE_EN.md)

NM-TV-154 是 NMTech 官方推出的小型智能显示屏（SmallTV），配备 1.54 英寸 240×240 彩屏。它既是一台加密彩票矿机（Hashrate 可达 1000+ KH/s，约为 NerdMiner 的 20 倍），也是一块通用的 **ESP32 开发板**，支持用户自行开发定制固件。

- 产品页：https://www.nmminer.com/product/nm-tv-154/
- 官方固件开发教程：https://www.nmminer.com/2026/03/02/how-to-develop-nm-tv-custom-firmware/
- 原理图：见官方教程中的 "NM-TV-154 Schematic"

---

## 1. 硬件规格

| 项目 | 规格 |
| --- | --- |
| 主控 | ESP32-WROOM-32（双核，240 MHz），PlatformIO 对应 `board = esp32dev` |
| Flash | 4 MB |
| Wi-Fi | 2.4 GHz 802.11 b/g/n |
| 显示屏 | 1.54 英寸 ST7789 TFT，240×240 分辨率，SPI 接口（HSPI） |
| 存储 | 384KB ROM；512KB SRAM；4MB Flash（SPIFFS / LittleFS） |
| 背光 | GPIO19 PWM 调光，**低电平点亮** |
| 输入 | 1 个电容触摸按键（ESP32 触摸通道 **T9**，即 GPIO32） |
| LCD 电源控制 | GPIO21 控制，**低电平使能** |
| 供电 / 烧录 | USB（板载 USB 转串口，115200 波特率） |

## 2. 引脚定义

### 2.1 显示屏（ST7789，SPI）

| 信号 | GPIO | 说明 |
| --- | --- | --- |
| TFT_MOSI | 13 | SPI 数据输出 |
| TFT_SCLK | 14 | SPI 时钟 |
| TFT_CS | 15 | 片选 |
| TFT_DC | 2 | 数据 / 命令选择 |
| TFT_RST | -1 | 未连接（使用软复位） |
| TFT_MISO | -1 | 未连接 |
| TFT_BL | 19 | 背光 PWM，低电平点亮 |
| — | 21 | LCD 电源轨开关，低电平使能 |

SPI 频率建议：

```
SPI_FREQUENCY       40000000
SPI_READ_FREQUENCY  20000000
```

### 2.2 触摸按键

| 项目 | 值 |
| --- | --- |
| 触摸通道 | T9（GPIO32） |
| 读取方式 | `touchRead(T9)` |
| 按下阈值 | 原始值 < 90（参考值，可按个体校准） |

### 2.3 电源与背光注意事项

1. **上电后必须先使能 LCD 电源轨**，否则屏幕无显示：

   ```cpp
   pinMode(21, OUTPUT);
   digitalWrite(21, LOW);   // LOW = 电源轨使能
   ```

2. **背光为低电平点亮**。使用 PWM 时占空比与亮度反相（duty 0 = 最亮，1023 = 熄灭）。参考实现（10 位分辨率、5 kHz）：

   ```cpp
   ledcSetup(0, 5000, 10);
   ledcAttachPin(19, 0);
   ledcWrite(0, 0);        // 最亮
   ledcWrite(0, 1023);     // 熄灭
   ```

3. **颜色反转**：本机 ST7789 需要 `tft.invertDisplay(true)` 才能显示正确颜色。

## 3. PlatformIO 配置要点

TFT_eSPI 的引脚定义通过 `platformio.ini` 的 `build_flags` 注入（`-DUSER_SETUP_LOADED=1`），**无需修改库目录下的 `User_Setup.h`**：

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

> 若在 Arduino IDE 中开发，则按官方教程修改 `Arduino/libraries/TFT_eSPI/User_Setup.h`，
> 并额外定义 `#define TFT_BL 19` 与 `#define TFT_BACKLIGHT_ON LOW`。

## 4. 烧录方式

### 4.1 串口烧录（PlatformIO）

```bash
pio run -e nm-tv-154 -t upload --upload-port COMx
```

如无法进入下载模式，重新上电进入烧录模式之后再试。

### 4.2 整片镜像烧录（esptool）

本工程编译后自动生成 `full_image.bin`（bootloader + 分区表 + 固件）：

```bash
esptool.py --chip esp32 write_flash 0x0 .pio/build/nm-tv-154/full_image.bin
```

该镜像也可用于工厂批量烧录或恢复官方固件之外的完整刷机。

## 5. 参考项目

- [RockBase-iot/deskbuddy-tv](https://github.com/RockBase-iot/deskbuddy-tv) —— 基于同一硬件的完整应用（天气 / 表情 / 时钟 / 网页配置 / OTA），其 HAL 层（`src/hal/nm_tv_154_hal.cpp`）的引脚定义与本工程一致，可作为进阶参考。
- [NMminer1024 (GitHub)](https://github.com/NMminer1024) —— 官方矿机固件。
