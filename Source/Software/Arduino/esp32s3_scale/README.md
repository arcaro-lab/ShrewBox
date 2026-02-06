# ESP32-S3 Digital Scale (NAU7802 + TFT Feather)

This project is an update for a legacy  
**Teensy 3.2 + TeensyView + load cell** digital scale, rewritten for the:

- **Adafruit TFT Feather ESP32-S3 (Reverse)**
- **NAU7802 24-bit ADC**
- **1 kg load cell**

It reads the load cell via the NAU7802, converts the value to **grams**, displays
the result on the built‑in TFT, and streams measurements over **USB Serial**.
A hardware button on **D0** is used for **tare**.

---

## Features

- 📏 24‑bit load cell reading via **NAU7802**
- ⚖️ Linear calibration: tare offset + scale factor
- 📺 Live weight + max weight display on TFT
- 🧮 Exponential smoothing (similar feel to the Teensy implementation)
- 🔘 **D1 button** for tare (INPUT_PULLDOWM, active‑high)
- 💻 USB Serial output for logging
- ⌨️ Serial commands compatible with the Teensy workflow

---

## Hardware

### Required
- Adafruit **TFT Feather ESP32‑S3 (Reverse)**
- **NAU7802** load cell amplifier
- 1 kg (or similar) load cell (Wheatstone bridge)

### Connections
| Signal | ESP32‑S3 Feather |
|------|------------------|
| NAU7802 SDA | SDA |
| NAU7802 SCL | SCL |
| NAU7802 VCC | 3V |
| NAU7802 GND | GND |
| Tare Button | D1 → GND |

> The NAU7802 uses I²C. No extra pins are required.

---

## Software Dependencies

Install the following libraries via **Arduino Library Manager**:

- **Adafruit NAU7802**
- **Adafruit GFX Library**
- **Adafruit ST7789**
- **Adafruit BusIO**

Board package:
- **Adafruit ESP32 Boards**  
  (select *Adafruit Feather ESP32‑S3 TFT*)

---

## Serial Commands

| Command | Action |
|-------|--------|
| `t` | Tare (zero the scale) |
| `s` | Start / stop streaming |

Baud rate: **115200**

---

## Calibration (Important)

Calibration is intentionally simple.

1. Upload the sketch
2. Open **Serial Monitor**
3. Ensure the scale is empty
4. Press **D1** (or send `t`) to tare
5. Place a **known weight** (e.g. 100 g)
6. Observe the printed values:
   ```
   RAW=xxxxx, TARE=yyyyy
   ```
7. Compute:
   ```
   CAL_G_PER_COUNT = known_grams / (RAW - TARE)
   ```
8. Edit the sketch:
   ```cpp
   static float CAL_G_PER_COUNT = 0.00012345;
   ```
9. Re-upload

---

## Display

The TFT shows:
- **Current weight (g)**
- **Maximum weight recorded**
- Tare offset and calibration factor (debug)

Update rate: ~4 Hz (configurable)

---

## Notes

- If readings appear inverted, either:
  - swap the load cell signal wires, or
  - leave wiring as-is (the code displays absolute weight)
- The smoothing factor can be adjusted:
  ```cpp
  static const float EMA_ALPHA = 0.20f;
  ```
- Power is fully USB-based

---

## License

MIT License  
Use freely for research, teaching, and lab instrumentation.

---

## Acknowledgments

- Adafruit NAU7802 driver
- Original Teensy-based scale implementation
