/*
  ESP32-S3 TFT Feather (Reverse) + NAU7802 load cell scale
  --------------------------------------------------------
  Replacement for Teensy 3.2 + TeensyView + TeensyLoadcell project.

  Features
  - Reads NAU7802 ADC (I2C) connected to a 1 kg load cell / bridge
  - Converts to grams using a simple linear calibration (tare offset + scale factor)
  - Displays current weight and max weight on the built-in TFT
  - Prints weight to Serial
  - Tare with:
      * Button on D1 (active-high, uses INPUT_PULLDOWN)
      * Serial command 't'
  - Start/stop streaming with Serial command 's' (like the Teensy sketch)

  Libraries
  - Adafruit_NAU7802
  - Adafruit_GFX
  - Adafruit_ST7789

  Calibration (IMPORTANT)
  1) Upload & open Serial Monitor.
  2) With nothing on the scale, press the D1 button (or send 't') to tare.
  3) Put a known weight (e.g. 100 g) on the scale.
  4) Read the printed "RAW" and "TARE" values and compute:

        scale_g_per_count = known_grams / (raw_with_weight - tare_raw)

     Put that value into CAL_G_PER_COUNT below.

  Notes
  - This code intentionally keeps calibration simple and transparent.
  - If you see negative values due to wiring polarity, either swap the load cell
    signal wires OR leave as-is and the code will absolute-value the reading.

*/

#include <Adafruit_NAU7802.h>

#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>

// TFT: the Feather TFT examples use these board-defined macros (TFT_CS, TFT_DC, TFT_RST, etc.)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// NAU7802
Adafruit_NAU7802 nau;

// --- User settings ---
static const uint32_t SERIAL_BAUD = 115200;

// Tare button (D1)
#ifndef PIN_TARE
#define PIN_TARE 1
#endif

// Display update period (ms)
static const uint32_t UI_PERIOD_MS = 100;

// Serial update period (ms)
static const uint32_t SERIAL_PERIOD_MS = 200;  // 5 Hz
static uint32_t last_serial_ms = 0;

// Smoothing (0 = no smoothing, closer to 1 = heavier smoothing)
// Original Teensy code used tau=0.2; this is "similar feel" in a simple EMA.
static const float EMA_ALPHA = 0.20f;

// Calibration parameters
// Set after you do the quick calibration procedure in the header comment.
static float CAL_G_PER_COUNT = 0.0009352f;// 0.000100f;  // <-- placeholder, must be tuned
static int32_t tare_raw = 0;

// --- State ---
static bool isRunning = true;
static float weight_g_filt = 0.0f;
static float max_g = 0.0f;

static uint32_t last_ui_ms = 0;

// ---------- Helpers ----------
static void tftHeader() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);

  // tft.setTextWrap(false);
  // tft.setTextColor(ST77XX_WHITE);
  // tft.setTextSize(2);
  // tft.setCursor(0, 0);
  // tft.print("Digital Scale");

  tft.setTextSize(1);
  tft.setCursor(0, 5);
  tft.print("D1: tare | Serial: 't' tare");
}

static void drawValuesFast(float g_now, float g_max) {
  // Use background color to overwrite previous text (no fillRect)
  tft.setTextWrap(false);

  // Line 1
  tft.setCursor(0, 50);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("Weight: ");
  char buf[16];
  snprintf(buf, sizeof(buf), "%4dg", (int)lroundf(g_now));  // pad spaces!
  tft.print(buf);

  // Line 2
  tft.setCursor(0, 95);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("Max:    ");
  snprintf(buf, sizeof(buf), "%4dg", (int)lroundf(g_max));
  tft.print(buf);
}


static int32_t readRawBlocking() {
  while (!nau.available()) {
    delay(1);
  }
  return nau.read();
}

static int32_t readRawAverage(uint8_t n) {
  int64_t acc = 0;
  for (uint8_t i = 0; i < n; i++) {
    acc += readRawBlocking();
  }
  return (int32_t)(acc / n);
}

static void tareNow() {
  // Take a small average for a stable tare
  tare_raw = readRawAverage(20);
  weight_g_filt = 0.0f;
  max_g = 0.0f;

  Serial.print("TARE set. tare_raw=");
  Serial.println(tare_raw);
}

static float rawToGrams(int32_t raw) {
  // Convert raw reading to grams
  float g = (float)(raw - tare_raw) * CAL_G_PER_COUNT;

  // Match original Teensy behavior: show absolute weight
  if (g < 0) g = -g;
  return g;
}

static bool tareButtonPressedDebounced() {
  // Simple debounce: detect stable low for ~30 ms
  static uint8_t stableCount = 0;
  if (digitalRead(PIN_TARE) == HIGH) {
    if (stableCount < 6) stableCount++;
  } else {
    stableCount = 0;
  }
  return (stableCount >= 6);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(50);

  // Button
  pinMode(PIN_TARE, INPUT_PULLDOWN);

  // TFT backlight and power rails (from Adafruit Feather TFT example)
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // TFT init
  tft.init(135, 240);         // ST7789 240x135'
  tft.setSPISpeed(80000000);  // 40 MHz
  tftHeader();


  // I2C for NAU7802
  Wire.begin();

  Serial.println();
  Serial.println("ESP32-S3 Scale + NAU7802");
  Serial.println("Commands: 't' tare, 's' start/stop");
  Serial.println("Tip: Calibrate CAL_G_PER_COUNT in the sketch header.");

  if (!nau.begin()) {
    Serial.println("ERROR: Failed to find NAU7802. Check wiring/I2C power.");
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.println("NAU7802 not found");
    while (1) delay(10);
  }
  Serial.println("Found NAU7802");


  // Configure NAU7802 similar to your example
  nau.setLDO(NAU7802_3V0);
  nau.setGain(NAU7802_GAIN_128);
  nau.setRate(NAU7802_RATE_320SPS);

  // Flush initial readings
  for (uint8_t i = 0; i < 10; i++) (void)readRawBlocking();

// Calibrate internal + offset
lead:
  while (!nau.calibrate(NAU7802_CALMOD_INTERNAL)) {
    Serial.println("Failed internal offset cal, retrying...");
    delay(200);
  }
  while (!nau.calibrate(NAU7802_CALMOD_OFFSET)) {
    Serial.println("Failed system offset cal, retrying...");
    delay(200);
  }
  Serial.println("NAU7802 calibrated");

  // Initial tare
  delay(300);
  tareNow();

  last_ui_ms = millis();
}

// ---------- Loop ----------
void loop() {
  // Serial commands (matches Teensy sketch)
  int c = Serial.read();
  if (c == 't' || c == 'T') {
    tareNow();
  } else if (c == 's' || c == 'S') {
    isRunning = !isRunning;
    Serial.print("Streaming: ");
    Serial.println(isRunning ? "ON" : "OFF");
  }

  // Button tare
  if (tareButtonPressedDebounced()) {
    // Wait for release so we don't retrigger continuously
    tareNow();
    while (digitalRead(PIN_TARE) == HIGH) delay(5);
  }

  if (!isRunning) {
    delay(10);
    return;
  }

  // Read sensor
  int32_t raw = readRawBlocking();
  float w_g = rawToGrams(raw);

  // Smooth
  weight_g_filt = weight_g_filt + EMA_ALPHA * (w_g - weight_g_filt);

  // Track max > 1 g (like your Teensy code)
  if (weight_g_filt > 1.0f && weight_g_filt > max_g) {
    max_g = weight_g_filt;
  }

  // Update UI + Serial periodically
  uint32_t now = millis();
  if (now - last_ui_ms >= UI_PERIOD_MS) {
    last_ui_ms = now;
    drawValuesFast(weight_g_filt, max_g);
  }
  if (now - last_serial_ms >= SERIAL_PERIOD_MS) {
    last_serial_ms = now;
    Serial.print("Weight ");
    Serial.print((int)lroundf(weight_g_filt));
    Serial.print(" g  (RAW=");
    Serial.print(raw);
    Serial.print(", TARE=");
    Serial.print(tare_raw);
    Serial.println(")");
  }
}
