# Hotel Room Thermostat - ESP32-S3 Smart HVAC Controller

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

## 📋 Project Overview

Advanced hotel room thermostat system built on ESP32-S3 with 4.0" capacitive touchscreen display. Features Modbus RTU communication, weather forecast integration, WiFi connectivity, and comprehensive hotel services management (DND, MUR, Clean Mode).

### Key Features

- 🌡️ **Precision Temperature Control**: NTC thermistor-based room temperature monitoring with 0.1°C resolution
- 🖥️ **4.0" Touchscreen Display**: ST7701 LCD with GT911 capacitive touch controller
- 📡 **Modbus RTU Slave**: Full-featured industrial communication protocol
- 🌤️ **Weather Forecast**: 5-day weather display with automatic data expiration
- 📶 **WiFi Manager**: Easy network configuration via captive portal
- 🔧 **3-Speed/Single Relay**: Configurable HVAC control modes
- 🏨 **Hotel Services**: Do Not Disturb, Make-Up Room, Clean Mode with UI lockout
- 💾 **Persistent Settings**: NVS flash storage for all configurations
- 🔒 **PIN Protection**: Secure access to advanced settings

---

## 🔧 Hardware Specifications

### Development Board
- **MCU**: ESP32-S3-WROOM-1-N8 (240MHz dual-core, 8MB Flash, 2MB PSRAM)
- **Display**: 4.0" ST7701 RGB LCD (480x480 resolution)
- **Touch**: GT911 I2C capacitive touch controller
- **Board**: ESP32-4848S040 (Waveshare compatible)

### GPIO Pin Assignments

| Pin | Function | Description |
|-----|----------|-------------|
| IO40 | Relay 1 | Heating/Cooling or Speed 1 |
| IO2 | Relay 2 | Fan Speed 2 |
| IO39 | Relay 3 | Fan Speed 3 |
| IO1 | ADC Input | NTC Thermistor 100kΩ (B3950) |
| IO41 | Digital Input | Window Sensor (Active Low) |
| IO48 | RS485 RTS | THVD1500 Transmit Enable |
| IO43 | UART0 TX | Serial Debug + Modbus (shared) |
| IO44 | UART0 RX | Serial Debug + Modbus (shared) |
| IO38 | PWM Output | LCD Backlight Control (0-100%) |
| IO19 | I2C SDA | GT911 Touch Controller |
| IO20 | I2C SCL | GT911 Touch Controller |

**⚠️ Critical Note**: UART0 (IO43/IO44) is **shared** between Serial debug output and Modbus RTU communication. Both run at **115200 baud** to prevent interference.

---

## 📡 Modbus RTU Communication

### Serial Configuration
- **Baudrate**: 115200 (8N1)
- **Protocol**: Modbus RTU Slave
- **Slave Address**: Configurable (1-247, default: 1)
- **RS485**: THVD1500 transceiver with IO48 as RTS

### Modbus Register Map

#### Holding Registers (40001+, Read/Write)
| Address | Register | Description | Range/Format |
|---------|----------|-------------|--------------|
| 40001 | Target Temperature | Setpoint (×10) | 100-400 (10.0-40.0°C) |
| 40002 | HVAC Mode | 0=OFF, 1=HEAT, 2=COOL | 0-2 |
| 40003 | Fan Speed | 0=AUTO, 1=LOW, 2=MID, 3=HIGH | 0-3 |
| 40004-40010 | Reserved | Future use | - |
| 40011 | Hour | System time hour | 0-23 |
| 40012 | Minute | System time minute | 0-59 |
| 40013 | Second | System time second | 0-59 |
| 40014 | Day | Day of month | 1-31 |
| 40015 | Month | Month of year | 1-12 |
| 40016 | Year | Full year | 2000-2099 |
| 40017 | Day of Week | 0=Sunday...6=Saturday | 0-6 |
| 40018-40021 | Reserved | Future use | - |
| 40022 | Relay Mode | 0=3-Speed, 1=Single | 0-1 |
| 40023-40029 | Reserved | Future use | - |
| 40030 | Weather Watchdog | Write to trigger update | Write any value |
| 40031-40045 | Weather Data | 5 days × 3 registers | See weather format |

**Weather Data Format (3 registers per day)**:
- `reg+0`: `day_id | (icon_id << 8)` (0=Sun, 1=Mon...6=Sat)
- `reg+1`: `temp_high_x10` (signed int16, °C×10)
- `reg+2`: `temp_low_x10` (signed int16, °C×10)

#### Input Registers (30001+, Read-Only)
| Address | Register | Description |
|---------|----------|-------------|
| 30001 | Current Temperature | Room temperature (×10) |
| 30002 | Target Temperature | Mirror of 40001 |
| 30003 | Relay Status | Bitmask: bit0=R1, bit1=R2, bit2=R3 |
| 30004 | Uptime Low | System uptime seconds (low 16 bits) |
| 30005 | Uptime High | System uptime seconds (high 16 bits) |
| 30006 | Free Heap | Available RAM in KB |
| 30007 | Window Sensor | Raw GPIO value (0=open, 1=closed) |

#### Coils (00001+, Read/Write)
| Address | Coil | Description |
|---------|------|-------------|
| 00001 | DND | Do Not Disturb flag |
| 00002 | MUR | Make-Up Room flag |

#### Discrete Inputs (10001+, Read-Only)
| Address | Input | Description |
|---------|-------|-------------|
| 10001 | Window Closed | TRUE if window closed |
| 10002 | System Ready | TRUE if system initialized |
| 10003 | HVAC Active | TRUE if heating/cooling active |
| 10004 | Weather Valid | TRUE if weather data not stale |

### Modbus Testing

Unified Python test script with multiple modes:

```bash
# Quick test (single cycle, all functions)
python test/modbus_test.py --quick

# Automated N-cycle test
python test/modbus_test.py --cycles 5

# Continuous test loop (Ctrl+C to stop)
python test/modbus_test.py --continuous

# Interactive menu
python test/modbus_test.py

# Custom port/slave address
python test/modbus_test.py --port COM10 --slave 2 --quick
```
```

**Requirements**: `pip install pymodbus pyserial colorama`

---

## 🎨 User Interface

Built with **LVGL 8.3.11** and **SquareLine Studio**. Exports located in `/lvgl` directory.

### Navigation Structure (Horizontal Swipe)
- **Tile 1 (Main)**: Clock, weather, inside/outside temperature
- **Tile 2 (Thermostat)**: Temperature control, HVAC mode, fan speed
- **Tile 3 (Weather)**: 5-day forecast (auto-hidden if stale >12h)
- **Tile 4 (Services)**: DND, MUR, Clean Mode, PIN-protected settings

### Special Features
- **Hidden Menu**: Long press (5s) top-left corner → PIN entry (default: 1234)
- **Inactivity Timeout**: 30s on Settings screens → return to Main
- **Clean Mode**: 60-second UI lockout for room cleaning
- **Window Alert**: Modal warning if HVAC attempted with open window

---

## 🛠️ Software Architecture

### Project Structure
```
HotelRoomThermostat/
├── src/
│   ├── main.cpp              # Main loop, LVGL tick, task scheduler
│   ├── hal.cpp               # Hardware abstraction (display, touch, GPIO)
│   ├── hvac.cpp              # HVAC logic, relay control, temperature control
│   ├── modbus_handler.cpp    # Modbus RTU implementation
│   ├── settings.cpp          # NVS storage, load/save preferences
│   └── wifi_manager.cpp      # WiFi config, captive portal
├── include/
│   ├── hal.h                 # HAL API definitions
│   ├── hvac.h                # HVAC control functions
│   ├── modbus_handler.h      # Modbus register map
│   ├── settings.h            # Settings structure definitions
│   ├── wifi_manager.h        # WiFi manager API
│   └── ESP32_4848S040.h      # Pin definitions for hardware
├── lvgl/
│   ├── ui.c/ui.h             # SquareLine Studio UI export
│   ├── ui_Main.c             # Main screen components
│   ├── ui_Settings*.c        # Settings screen components
│   ├── ui_PinEntry.c         # PIN keypad screen
│   └── assets/               # Images, fonts
├── test/
│   ├── modbus_test.py        # Unified Modbus test suite (3 modes)
│   └── README_MODBUS_TEST.md # Test documentation
├── docs/
│   ├── FSD.md                # Functional specification (Serbian)
│   ├── DIAGRAMS.md           # System architecture diagrams
│   └── tasks/                # Development task breakdown
└── platformio.ini            # PlatformIO project config
```

### Key Libraries
- **LVGL 8.3.11**: Graphics and UI framework
- **GFX Library for Arduino 1.4.9**: Display driver abstraction
- **modbus-esp8266 4.1.0**: Modbus RTU stack (ESP32 compatible)
- **ArduinoJson 6.21.6**: JSON parsing for WiFi config
- **Touch GT911**: Custom I2C touch driver

---

## 🚀 Build & Upload

### Prerequisites
- **PlatformIO Core** or **PlatformIO IDE** (VS Code extension)
- **Python 3.7+** (for Modbus testing)

### Build Commands
```bash
# Build firmware
platformio run

# Upload to ESP32-S3
platformio run --target upload

# Open serial monitor
platformio device monitor
```

### Configuration
Edit `platformio.ini` for custom settings:
```ini
[env:thermostat]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
```

---

## 📊 System Operation

### Temperature Control Logic
1. **Sensor Reading**: NTC thermistor (IO1) → voltage divider → ADC1_CH0
2. **Steinhart-Hart Equation**: Convert resistance to temperature
3. **Control Loop**: Every 5 seconds, compare current vs. target temp
4. **Relay Activation**: 
   - **3-Speed Mode**: OFF → 100ms delay → Speed 1/2/3
   - **Single Mode**: Simple ON/OFF control
5. **Safety**: Window sensor (IO41) overrides HVAC → force OFF

### Modbus Response Timing
- **Critical**: No `Serial.print()` allowed during Modbus callbacks
- **Reason**: Shared UART0 port causes response timeouts
- **Solution**: Debug logging disabled in `cb_hreg_write()` callback

### Weather Data Lifecycle
1. Master writes `40030` (watchdog trigger) + forecast data
2. ESP32 parses 5 days × 3 registers into UI structure
3. Tile 3 (Weather) displays forecast
4. After 12 hours without update → `weather_stale = true`
5. Auto-hide Tile 3 from swipe navigation

---

## 🔐 Security Features

- **PIN Protection**: Default 1234, changeable in settings
- **NVS Encryption**: Sensitive data stored in encrypted flash partition
- **WiFi Credentials**: Stored securely via WifiManager library
- **Modbus Address Lock**: Requires "Save & Exit" to persist changes

---

## 📝 Development Notes

### Known Issues & Workarounds
1. **UART0 Sharing**: 
   - **Issue**: Debug logs + Modbus on same port
   - **Fix**: 115200 baud for both, no logging in callbacks
   
2. **Modbus Write Timeouts**:
   - **Issue**: ~5% write operations timeout
   - **Behavior**: Writes succeed but response delayed → retry works
   - **Acceptable**: Inherent limitation of shared serial port

3. **Temperature Reading Anomaly**:
   - **Issue**: Reads 6500+°C when NTC disconnected
   - **Cause**: ADC reads rail voltage (saturated)
   - **Solution**: Add sensor validation in `hvac.cpp`

### Future Enhancements
- [ ] Auto-save Modbus address on change (no "Save & Exit" required)
- [ ] MQTT/HTTP weather data fetch (eliminate Modbus dependency)
- [ ] Touch calibration screen
- [ ] Multi-language support (i18n via lv_i18n.h)
- [ ] Energy consumption monitoring

---

## 📄 License

MIT License - See LICENSE file for details

---

## 🤝 Contributing

Pull requests welcome! Please ensure:
1. Code follows existing style conventions
2. Modbus register map changes are documented
3. Test scripts updated for new features
4. FSD.md synchronized with code changes

---

## 📞 Support

For issues, questions, or feature requests:
- Open an issue on GitHub
- Check `/docs` folder for detailed specifications
- Review Modbus test output for communication troubleshooting

---

## 🙏 Acknowledgments

- **LVGL Team**: Excellent embedded graphics library
- **SquareLine Studio**: Professional UI design tool
- **modbus-esp8266**: Robust Modbus stack
- **Espressif**: ESP32-S3 SDK and documentation

---

**Project Status**: ✅ Production Ready (v1.0.0)  
**Last Updated**: April 2026  
**Build Status**: ![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
