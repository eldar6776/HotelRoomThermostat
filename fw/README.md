# Hotel Room Thermostat — Firmware

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)
![Framework](https://img.shields.io/badge/framework-Arduino%2FPlatformIO-orange.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

Advanced hotel room thermostat firmware for ESP32-S3 with 4.0" capacitive touchscreen, Modbus RTU communication, HVAC control, weather forecast integration, and full hotel services management (DND, MUR, Clean Mode).

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32-S3-WROOM-1-N8, 240 MHz dual-core, 8 MB Flash, 8 MB OPI PSRAM |
| Display | 4.0" ST7701 RGB LCD, 480×480 |
| Touch | GT911 capacitive touch controller (I2C0: IO19/IO45) |
| GPIO Expander | PCF8574AN (test) → PCA9554ADH (production) via I2C1: IO2/IO40 |
| Temperature | NTC 100 kΩ B3950 thermistor on ADC1_CH0 (IO1) |
| RS485 | THVD1500 transceiver, DE on IO41 |
| Backlight | PWM on IO38 (0–100%) |

### GPIO Pin Map

| Pin | Function |
|-----|----------|
| IO1 | NTC thermistor (ADC) |
| IO2 | I2C1 SDA → GPIO Expander |
| IO19 | I2C0 SDA → GT911 Touch |
| IO38 | Backlight PWM |
| IO40 | I2C1 SCL → GPIO Expander |
| IO41 | RS485 DE (Driver Enable) |
| IO43 | UART0 TX — Serial debug + Modbus (shared, 115200 baud) |
| IO44 | UART0 RX — Serial debug + Modbus (shared, 115200 baud) |
| IO45 | I2C0 SCL → GT911 Touch |

### GPIO Expander Pin Map

| Pin | Function | Direction |
|-----|----------|-----------|
| P0 | Relay 1 (Heat/Cool or Fan Speed 1) | Output |
| P1 | Relay 2 (Fan Speed 2) | Output |
| P2 | Relay 3 (Fan Speed 3) | Output |
| P3–P5 | Reserved outputs (ULN2003) | Output |
| P6 | Window sensor (Active LOW) | Input |
| P7 | Reserved input | Input |

> **PCF8574AN** — open-drain, inverted logic (`0` = relay ON), not glitch-free. Requires 10 kΩ pull-down on P0–P2. I2C address: `0x38`.
>
> **PCA9554ADH** — push-pull, direct logic (`1` = relay ON), glitch-free power-on. I2C address: `0x20`.

---

## Build & Upload

Prerequisites: **PlatformIO** (VS Code extension or CLI), Python 3.7+ for testing.

```bash
# Build
platformio run

# Upload
platformio run --target upload

# Serial monitor
platformio device monitor
```

Flash layout: `huge_app` partition (16 MB flash, `qio_opi` PSRAM mode).

---

## Modbus RTU

**Slave, 115200 8N1.** Slave address configurable (1–247, default: 1), stored in NVS.

> ⚠️ UART0 is shared between Serial debug and Modbus. Never use `Serial.print()` inside Modbus callbacks — causes response timeouts.

### Holding Registers (40001+, R/W)

| Address | Description | Range |
|---------|-------------|-------|
| 40001 | Target temperature (×10) | 100–400 (10.0–40.0 °C) |
| 40002 | HVAC mode: 0=OFF, 1=HEAT, 2=COOL | 0–2 |
| 40003 | Fan speed: 0=AUTO, 1=LOW, 2=MID, 3=HIGH | 0–3 |
| 40011–40017 | Time sync: Hour, Min, Sec, Day, Month, Year, DoW | — |
| 40022 | Relay mode: 0=3-speed, 1=single | 0–1 |
| 40030 | Weather watchdog (write any value to trigger) | — |
| 40031–40045 | Weather data: 5 days × 3 registers | See below |

Weather register format (per day): `reg+0` = `day_id \| (icon_id << 8)`, `reg+1` = high temp ×10, `reg+2` = low temp ×10.

### Input Registers (30001+, Read Only)

| Address | Description |
|---------|-------------|
| 30001 | Current temperature (×10) |
| 30003 | Relay status bitmask (bit0=R1, bit1=R2, bit2=R3) |
| 30004–30005 | Uptime seconds (low/high 16 bits) |
| 30006 | Free heap (KB) |
| 30007 | Window sensor raw (0=open, 1=closed) |

### Coils (00001+, R/W)

| Address | Description |
|---------|-------------|
| 00001 | DND — Do Not Disturb |
| 00002 | MUR — Make Up Room |

### Modbus Test Script

```bash
# Quick test (single cycle)
python test/modbus_test.py --quick

# N-cycle automated test
python test/modbus_test.py --cycles 5

# Continuous loop (Ctrl+C to stop)
python test/modbus_test.py --continuous

# Custom port/address
python test/modbus_test.py --port COM10 --slave 2 --quick
```

Dependencies: `pip install pymodbus pyserial colorama`

---

## User Interface

Built with **LVGL 8.3.11** + **SquareLine Studio**. UI exports in `lvgl/`.

### Screen Navigation (3-screen endless loop)

```
Main ──► Thermostat ──► Clean ──► (back to Main)
```

| Screen | Function |
|--------|----------|
| **Main** | Clock, inside/outside temperature, status icons |
| **Thermostat** | Arc drag for setpoint (10–40 °C), HVAC mode, fan speed cycle |
| **Clean** | 60-second UI lockout with countdown timer |

### Special Behaviors

| Feature | Detail |
|---------|--------|
| Hidden settings menu | Long press (5 s) top-left corner → PIN entry (default: 1234) |
| Inactivity timeout | 30 s on any Settings screen → return to Main |
| Window alert | Modal warning if HVAC activated with open window |
| DND / MUR | Hotel service flags, controlled via UI or Modbus coils |

---

## HVAC Logic

- **Temperature**: NTC → Steinhart-Hart → EMA filter → 0.1 °C resolution
- **Control loop**: runs every 5 s, dead-band hysteresis configurable
- **Relay deadband**: mandatory 100 ms all-OFF between speed transitions
- **Fan Schmitt trigger**: `fan_diff` narrowed to 0.2 °C to prevent speed oscillation
- **Window sensor**: P6 LOW → HVAC forced OFF regardless of setpoint
- **Relay modes**: 3-speed fan (R1/R2/R3) or single relay (R1 only)

---

## Settings & NVS

Access via PIN (default: **1234**).

| Screen | Parameters |
|--------|-----------|
| Settings 1 | Min/Max temp, HVAC mode, control type |
| Settings 2 | Hysteresis, stage step, sensor offset |
| Settings 3 | Backlight brightness, timeout, Modbus ID, WiFi AP switch |

All changes are buffered in RAM using a **dirty-flag bitmask** (`settings_flag_t`). NVS write occurs only on "Save & Exit", and only for modified parameters.

---

## WiFi / OTA

WiFi Manager activates as a captive-portal AP via Settings 3. While AP is active, the inactivity timeout is paused. Firmware updates are performed exclusively through this mode.

---

## Key Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| LVGL | 8.3.11 | Graphics / UI framework |
| GFX Library for Arduino | 1.4.9 | ST7701 display driver |
| modbus-esp8266 | 4.1.0 | Modbus RTU stack |
| ArduinoJson | 6.21.6 | JSON / WiFi config parsing |
| Touch GT911 | custom | I2C capacitive touch driver |

---

## Project Structure

```
fw/
├── src/
│   ├── main.cpp              # Main loop, LVGL tick, task scheduler
│   ├── hal.cpp               # Hardware abstraction (display, touch, GPIO expander)
│   ├── hvac.cpp              # HVAC logic, relay control, temperature
│   ├── modbus_handler.cpp    # Modbus RTU slave implementation
│   ├── settings.cpp          # NVS load/save, dirty-flag management
│   └── wifi_manager.cpp      # WiFi captive portal
├── include/
│   ├── hal.h                 # HAL API + EXPANDER_TYPE define
│   ├── hvac.h                # HVAC control API
│   ├── modbus_handler.h      # Register map definitions
│   ├── settings.h            # Settings structure + flag enum
│   ├── wifi_manager.h        # WiFi manager API
│   └── ESP32_4848S040.h      # Board pin definitions
├── lvgl/
│   ├── ui.c / ui.h           # SquareLine Studio root export
│   ├── ui_Main.c             # Main screen
│   ├── ui_Thermostat.c       # Thermostat screen
│   ├── ui_Clean.c            # Clean mode screen
│   ├── ui_Settings1-3.c      # Settings screens
│   ├── ui_PinEntry.c         # PIN keypad
│   ├── ui_events.c           # UI event handlers
│   └── assets/               # Fonts, icons, images
├── test/
│   ├── modbus_test.py        # Unified Modbus test suite
│   └── README_MODBUS_TEST.md
├── docs/
│   ├── FSD.md                # Full functional specification (Serbian)
│   ├── DIAGRAMS.md           # Mermaid architecture diagrams
│   └── tasks/                # Development task breakdown
└── platformio.ini
```

---

## License

MIT — see [LICENSE](LICENSE).
