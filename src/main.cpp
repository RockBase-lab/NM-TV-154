/**
 * NM-TV-154 PlatformIO Demo
 *
 * An open-source demo firmware for the NMMiner NM-TV-154 development board.
 *
 * Hardware:
 *   - ESP32 (esp32dev, 240 MHz dual core)
 *   - 1.54" ST7789 240x240 TFT (SPI, HSPI port)
 *   - Capacitive touch key (ESP32 touch pad T9 / GPIO32)
 *   - PWM dimmable backlight (GPIO19, active LOW)
 *   - LCD power rail switch (GPIO21, LOW = rail enabled)
 *
 * Features demonstrated:
 *   1. ST7789 display init via TFT_eSPI (pins configured in platformio.ini,
 *      no need to edit User_Setup.h)
 *   2. Drawing primitives, centered text and accent colors
 *   3. Capacitive touch reading with tap counting
 *   4. PWM backlight control with brightness levels
 *   5. Serial log output (115200 baud)
 *
 * References:
 *   - https://www.nmminer.com/2026/03/02/how-to-develop-nm-tv-custom-firmware/
 *   - https://github.com/RockBase-iot/deskbuddy-tv
 */

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// ---------------------------------------------------------------------------
// NM-TV-154 pinout
// ---------------------------------------------------------------------------
static const uint8_t PIN_LCD_PWR = 21;  // LCD power rail, LOW = enabled
static const uint8_t PIN_LCD_BL = 19;   // backlight PWM, active LOW
static const uint8_t PIN_TOUCH = T9;    // capacitive touch key (GPIO32)

static const uint16_t TOUCH_THRESHOLD = 90;   // raw value below => pressed

// Backlight PWM (backlight is active LOW, so duty 1023 = off)
static const uint8_t BL_LEDC_CHAN = 0;
static const uint16_t BL_LEDC_FREQ = 5000;
static const uint8_t BL_LEDC_RES = 10;  // 10-bit, 0..1023

// Brightness levels 0..4 (4 = brightest)
static const uint8_t BL_LEVEL_MAX = 4;

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
#define NM_RED 0xF800
#define NM_BLUE 0x001F
#define NM_YELLOW 0xFFE0
#define NM_GREEN 0x07E0
#define NM_CYAN 0x07FF
#define NM_MAGENTA 0xF81F
#define NM_BLACK 0x0000
#define NM_WHITE 0xFFFF

static const uint16_t ACCENT_COLORS[] = {NM_BLUE, NM_GREEN, NM_MAGENTA, NM_CYAN, NM_YELLOW, NM_RED};
static const uint8_t ACCENT_COUNT = sizeof(ACCENT_COLORS) / sizeof(ACCENT_COLORS[0]);

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
TFT_eSPI tft = TFT_eSPI();

static uint8_t blLevel = BL_LEVEL_MAX;
static uint8_t accentIndex = 0;
static uint32_t tapCount = 0;
static uint32_t longPressCount = 0;

// ---------------------------------------------------------------------------
// Backlight
// ---------------------------------------------------------------------------
void setBacklightLevel(uint8_t level) {
    if (level > BL_LEVEL_MAX) level = BL_LEVEL_MAX;
    blLevel = level;
    // Active LOW: full brightness => duty 0, off => duty 1023
    uint32_t duty = (uint32_t)((BL_LEVEL_MAX - level) * 200u);
    if (duty > 1023) duty = 1023;
    ledcWrite(BL_LEDC_CHAN, duty);
}

// ---------------------------------------------------------------------------
// UI rendering
// ---------------------------------------------------------------------------
void drawHeader(uint16_t color) {
    tft.fillRect(0, 0, 240, 44, color);
    tft.drawFastHLine(0, 44, 240, NM_RED);
    tft.setTextColor(NM_WHITE, color);
    tft.drawCentreString("NM-TV-154 DEMO", 120, 14, 2);
}

void drawStatus() {
    uint16_t accent = ACCENT_COLORS[accentIndex];
    drawHeader(accent);

    // Clear body area
    tft.fillRect(0, 45, 240, 240 - 45, NM_BLACK);

    tft.setTextColor(NM_YELLOW, NM_BLACK);
    tft.drawCentreString("PlatformIO + TFT_eSPI", 120, 70, 2);

    tft.setTextColor(NM_WHITE, NM_BLACK);
    tft.drawCentreString("Tap the touch key!", 120, 100, 2);

    // Tap counter
    char buf[40];
    snprintf(buf, sizeof(buf), "Taps: %lu", (unsigned long)tapCount);
    tft.setTextColor(accent, NM_BLACK);
    tft.drawCentreString(buf, 120, 140, 4);

    // Backlight level bar
    tft.setTextColor(NM_WHITE, NM_BLACK);
    tft.drawCentreString("Backlight", 120, 175, 2);
    for (uint8_t i = 0; i <= BL_LEVEL_MAX; i++) {
        uint16_t c = (i <= blLevel) ? accent : 0x4208;  // filled or dark grey
        tft.fillRect(52 + i * 28, 195, 22, 12, c);
    }

    tft.setTextColor(0x7BEF, NM_BLACK);
    tft.drawCentreString("tap: color | hold: dim", 120, 220, 1);
}

// ---------------------------------------------------------------------------
// Touch handling (simple tap / long-press detection)
// ---------------------------------------------------------------------------
static const uint32_t LONG_PRESS_MS = 800;
static bool touchDown = false;
static uint32_t touchDownMs = 0;
static bool longPressEmitted = false;

bool touchPressed() {
    return touchRead(PIN_TOUCH) < TOUCH_THRESHOLD;
}

void pollTouch(uint32_t now) {
    bool pressed = touchPressed();

    if (pressed && !touchDown) {
        // Falling edge: touch started
        touchDown = true;
        touchDownMs = now;
        longPressEmitted = false;
        Serial.printf("[touch] pressed, raw=%d\n", touchRead(PIN_TOUCH));
    } else if (pressed && touchDown && !longPressEmitted &&
               (now - touchDownMs) >= LONG_PRESS_MS) {
        // Long press: dim / brighten the backlight
        longPressEmitted = true;
        longPressCount++;
        setBacklightLevel((blLevel + BL_LEVEL_MAX) % (BL_LEVEL_MAX + 1));  // step down, wrap
        Serial.printf("[touch] long press -> backlight level %u\n", blLevel);
        drawStatus();
    } else if (!pressed && touchDown) {
        // Rising edge: touch released
        touchDown = false;
        if (!longPressEmitted) {
            // Short tap: cycle accent color
            tapCount++;
            accentIndex = (accentIndex + 1) % ACCENT_COUNT;
            Serial.printf("[touch] tap #%lu -> accent %u\n",
                          (unsigned long)tapCount, accentIndex);
            drawStatus();
        }
    }
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("[boot] NM-TV-154 PlatformIO demo");

    // Enable the LCD power rail
    pinMode(PIN_LCD_PWR, OUTPUT);
    digitalWrite(PIN_LCD_PWR, LOW);
    Serial.println("[boot] LCD power rail enabled");
    delay(50);

    // Backlight PWM
    ledcSetup(BL_LEDC_CHAN, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(PIN_LCD_BL, BL_LEDC_CHAN);
    Serial.println("[boot] Backlight PWM attached");

    // Display
    tft.begin();
    tft.setSwapBytes(true);
    tft.setRotation(0);
    tft.invertDisplay(true);  // ST7789 on NM-TV-154 needs inversion for correct colors
    tft.fillScreen(NM_BLACK);
    Serial.println("[boot] ST7789 initialized");

    setBacklightLevel(BL_LEVEL_MAX);

    // Splash screen
    tft.fillScreen(NM_YELLOW);
    tft.fillRect(0, 0, 240, 50, NM_BLUE);
    tft.drawFastHLine(0, 50, 240, NM_RED);
    tft.setTextColor(NM_BLACK, NM_YELLOW);
    tft.drawCentreString("NM-TV-154", 120, 120, 4);
    delay(1200);

    drawStatus();
    Serial.printf("[boot] ready, touch raw=%d\n", touchRead(PIN_TOUCH));
}

void loop() {
    pollTouch(millis());
    delay(10);
}
