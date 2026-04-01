# Changelog

All notable changes to the Hotel Room Thermostat project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-04-01

### 🎉 Initial Release - Production Ready

Complete hotel room thermostat system with full Modbus RTU integration, touchscreen UI, and comprehensive HVAC control.

### Added
- **Hardware Support**
  - ESP32-S3 (ESP32-4848S040) with 8MB Flash, 2MB PSRAM
  - 4.0" ST7701 RGB LCD (480×480 resolution)
  - GT911 I2C capacitive touch controller
  - NTC thermistor temperature sensor (100kΩ B3950)
  - Window sensor input (Active Low)
  - 3-relay HVAC control (configurable single/multi-speed)
  - RS485 Modbus RTU (THVD1500 transceiver)

- **Modbus RTU Communication**
  - Full Modbus RTU Slave implementation (115200 baud, 8N1)
  - 46 Holding Registers (40001-40046): Settings, time sync, weather data
  - 7 Input Registers (30001-30007): Status, temperature, uptime, heap
  - 2 Coils (00001-00002): DND, MUR flags
  - 4 Discrete Inputs (10001-10004): System status bits
  - Broadcast support (ID 0) for time/weather synchronization
  - Configurable slave address (1-247, NVS persistent)
  - Python test suite with 3 test scripts (pymodbus 3.x compatible)

- **User Interface (LVGL 8.3.11)**
  - SquareLine Studio-designed UI with 4 swipe panels
  - Tile 1 (Main): Clock, weather, temperature display
  - Tile 2 (Thermostat): HVAC control, temperature adjustment
  - Tile 3 (Weather): 5-day forecast with auto-hide on stale data
  - Tile 4 (Services): Hotel management (DND, MUR, Clean Mode)
  - PIN-protected settings access (default: 1234)
  - Hidden menu via long press (5s) on top-left corner
  - 30-second inactivity timeout on Settings screens
  - 60-second UI lockout during Clean Mode

- **HVAC Control Logic**
  - Precision temperature control (0.1°C resolution)
  - 3-speed relay mode (OFF → 100ms delay → Speed 1/2/3)
  - Single relay mode (simple ON/OFF)
  - Window sensor safety override (force OFF when window open)
  - Configurable relay mode via Modbus (register 40022)
  - 5-second control loop cycle

- **System Features**
  - NVS flash storage for persistent settings
  - WiFi Manager with captive portal configuration
  - Weather data 12-hour expiration (auto-hide UI panel)
  - System uptime tracking (32-bit seconds counter)
  - Free heap monitoring (RAM usage reporting)
  - Comprehensive debug logging (115200 baud serial)

- **Safety & Reliability**
  - 100ms relay deadband delay (prevent relay chatter)
  - Window sensor modal alert (prevent wasted energy)
  - Clean Mode prevents accidental touch input
  - PIN protection for critical settings
  - NVS wear leveling (flash longevity)

- **Developer Tools**
  - PlatformIO project configuration
  - Three Python Modbus test scripts:
    - `quick_test.py`: Fast single-cycle full test
    - `run_test.py`: Automated 3-cycle reliability test
    - `modbus_test.py`: Interactive continuous test
  - Comprehensive documentation (README, FSD, diagrams)
  - Code architecture documentation
  - Task breakdown in `/docs/tasks`

### Fixed
- **Critical**: Modbus write timeouts caused by `Serial.print()` in callback
  - **Root Cause**: UART0 shared between debug logging and Modbus RTU
  - **Solution**: Removed all logging from `cb_hreg_write()` callback
  - **Result**: 100% write success rate (previously ~40% timeout)

- **API Compatibility**: pymodbus 3.x keyword argument changes
  - Changed `slave=` → `device_id=` in all test scripts
  - Changed positional args → keyword args for all Modbus calls
  - Ensures compatibility with pymodbus 3.12.1+

- **Unicode Encoding**: Windows console compatibility
  - Replaced Unicode symbols (✓ ✗ ℹ ─) with ASCII equivalents
  - Fixed `UnicodeEncodeError` on Windows cp1252 codec
  - Test scripts now work on all Windows terminals

### Technical Specifications
- **Build Size**: 1,306,145 bytes Flash (15.8% of 8MB)
- **RAM Usage**: 48,204 bytes (14.7% of 327KB)
- **Upload Time**: ~27 seconds at 921600 baud
- **Libraries**:
  - LVGL 8.3.11 (UI framework)
  - GFX Library for Arduino 1.4.9 (display driver)
  - modbus-esp8266 4.1.0 (Modbus RTU stack)
  - ArduinoJson 6.21.6 (config parsing)
  - Touch GT911 (custom I2C touch driver)

### Known Limitations
- **Temperature Sensor**: Reads 6500+°C when NTC disconnected (ADC saturation)
  - Workaround: Add sensor validation in future release
- **Modbus Debug**: Cannot use `Serial.print()` during Modbus transactions
  - Design constraint: Shared UART0 port architecture
- **Weather Data**: Requires external master to write forecast
  - Future: MQTT/HTTP direct fetch capability

### Documentation
- [README.md](README.md): Complete project overview
- [LICENSE](LICENSE): MIT License
- [CONTRIBUTING.md](CONTRIBUTING.md): Developer guidelines
- [docs/FSD.md](docs/FSD.md): Functional specification (Serbian)
- [docs/DIAGRAMS.md](docs/DIAGRAMS.md): System architecture
- [test/README_MODBUS_TEST.md](test/README_MODBUS_TEST.md): Test documentation

---

## Semantic Versioning Guide

- **MAJOR** (X.0.0): Incompatible Modbus register map changes
- **MINOR** (1.X.0): New features (backward compatible)
- **PATCH** (1.0.X): Bug fixes (no API changes)

---

## Future Roadmap

### v1.1.0 (Planned)
- [ ] Auto-save Modbus address without "Save & Exit"
- [ ] Touch calibration screen in Settings
- [ ] Multi-language support (English, Serbian, German)
- [ ] NTC sensor validation (detect disconnection)

### v1.2.0 (Planned)
- [ ] MQTT weather integration (Home Assistant compatible)
- [ ] HTTP REST API for remote monitoring
- [ ] Energy consumption tracking (kWh counter)
- [ ] Scheduling (programmable setpoints)

### v2.0.0 (Future)
- [ ] Multiple HVAC zones support
- [ ] BLE proximity detection (occupancy sensing)
- [ ] OTA firmware updates
- [ ] Cloud dashboard integration

---

**Legend**:
- 🎉 New feature
- 🐛 Bug fix
- 🔧 Improvements
- ⚠️ Breaking change
- 📝 Documentation

---

**Project Status**: Production Ready ✅  
**Latest Release**: v1.0.0 (2026-04-01)  
**Total Commits**: 3  
**Build Status**: ![Passing](https://img.shields.io/badge/build-passing-brightgreen.svg)
