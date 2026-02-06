# 3-Channel Stepper Motor Controller (TMC2209 + Serial)

This project implements a **3-stepper-motor controller** using **TMC2209 step/dir drivers**, controlled either by **physical toggle switches** or by **serial commands** over USB.

It is designed for **simple, reliable lab use**, supporting both manual jogging and scripted motion from MATLAB, Python, or a serial terminal.

---

## Features

- Control **3 independent stepper motors**
- Dual control modes:
  - **Manual toggle switches** (LEFT / RIGHT per motor)
  - **Serial control** over USB
- Simple human-readable command format  
  (`S1,0,100`, `S2,1,200,400`)
- Compatible with **TMC2209 in STEP/DIR mode**
- Deterministic blocking motion (easy to reason about)

---

## Hardware Assumptions

- Microcontroller with USB serial (nRF52 / TinyUSB compatible)
- Stepper drivers: **TMC2209** (standalone STEP/DIR mode)
- Enable pin is **active-LOW**
- Toggle switches wired to **GND**, using `INPUT_PULLUP`

---

## Pin Mapping

### Stepper Drivers

| Motor | DIR | STEP | EN |
|------:|----:|-----:|---:|
| S1 | D4 | D5 | D6 |
| S2 | D2 | D13 | D3 |
| S3 | D0 | D11 | D1 |

### Toggle Switches

| Motor | LEFT | RIGHT |
|------:|-----:|------:|
| S1 | D19 | D7 |
| S2 | D18 | D8 |
| S3 | D10 | D9 |

Switch inputs are **active LOW**.

---

## Motion Parameters

Defined in firmware:

```cpp
int defaultSteps = 100;   // steps per button press
int defaultSpUs  = 400;  // microseconds HIGH + LOW per step
```

- `sp_us = 400` → ~1250 steps/sec
- Smaller `sp_us` → faster motion

Each step pulse:
- HIGH for `sp_us`
- LOW  for `sp_us`

---

## Serial Interface

- **Baud rate:** `57600`
- ASCII, line-based commands
- Commands are **case-insensitive**
- Newline (`\n`) or carriage return (`\r`) ends a command

---

## Serial Command Format

### Basic Move

```
S<motor>,<dir>,<steps>
```

Example:
```
S1,0,100
```

Meaning:
- Motor 1
- Direction = 0 (clockwise)
- 100 steps
- Uses default speed

---

### Move With Speed Control

```
S<motor>,<dir>,<steps>,<sp_us>
```

Example:
```
S2,1,200,300
```

Meaning:
- Motor 2
- Direction = 1 (anti-clockwise)
- 200 steps
- 300 µs HIGH + 300 µs LOW per step

---

## Direction Convention

| Value | Meaning | DIR pin |
|-----:|--------|--------|
| `0` | Clockwise (CW) | LOW |
| `1` | Anti-clockwise (CCW) | HIGH |

> If your mechanical direction is reversed, swap `HIGH` / `LOW` in `setDir()`.

---

## Manual Toggle Switch Operation

- Press **LEFT** → motor moves CCW
- Press **RIGHT** → motor moves CW
- Each press moves `defaultSteps` steps
- Serial commands remain active while using buttons

---

## Example Usage

### Serial Terminal
```
S1,0,100
S2,1,500,250
S3,0,1000
```

### Python
```python
import serial
ser = serial.Serial("/dev/ttyACM0", 57600)
ser.write(b"S1,0,100\n")
```

### MATLAB
```matlab
s = serialport("/dev/ttyACM0",57600);
writeline(s,"S2,1,200,400");
```

---

## Design Notes

- Motion is **blocking**
  - Simple, predictable behavior
  - Motors do not move simultaneously
- No acceleration or ramping
- No absolute position tracking (open-loop)
- Well suited for:
  - Calibration rigs
  - Syringe pumps
  - Optical positioning
  - Behavioral experiment hardware

---

## File List

- `stepper_1.0.ino` — main firmware
- `README.md` — documentation

---

## Future Improvements (Optional)

- Non-blocking motion (simultaneous motors)
- Acceleration profiles
- Homing and limit switches
- Absolute positioning
- Command queueing
- Enable/disable motors via serial
