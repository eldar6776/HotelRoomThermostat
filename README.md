# Hotel Room Thermostat

Unified repository for the hotel room thermostat system:
- Firmware for ESP32-S3 touchscreen thermostat (`fw/`)
- Hardware design files for custom wall base board (`hw/`)

This root README is the main entry point for the full project (firmware + hardware).

## Repository Layout

- `fw/` Firmware project (PlatformIO, LVGL UI, Modbus RTU, HVAC logic)
- `hw/` Hardware project files (schematic, PCB, project files, outputs)
- `doc/` Additional documentation and shared notes
- `sw/` Reserved for supporting software/tools

## Firmware Summary (`fw/`)

The firmware targets ESP32-S3 (4848S040 style board) with:
- 4.0" ST7701 display + GT911 touch
- Modbus RTU slave communication
- HVAC control (heat/cool/fan modes)
- Weather data integration
- Settings persistence in NVS

Main firmware entry and docs:
- `fw/src/main.cpp`
- `fw/README.md`
- `fw/docs/FSD.md`
- `fw/docs/DIAGRAMS.md`

### Build and Upload (Firmware)

From `fw/` directory:

```bash
platformio run
platformio run --target upload
platformio device monitor
```

## Hardware Summary (`hw/`)

Hardware folder contains the custom thermostat board design package, including:
- Project files (`.PrjPcb`, schematic, PCB, libraries)
- Board revisions and production-related outputs

Current primary board path:
- `hw/WallBaseBoard/`

Recommended versioned source files to keep under git:
- project file(s)
- schematic(s)
- pcb file(s)
- symbol/footprint libraries

Generated artifacts (history snapshots, project logs, fabrication exports) are ignored via root `.gitignore` to keep repository history clean.

## Git Workflow Recommendation

For clean history, use separate commits for:
1. Firmware code changes (`fw/src`, `fw/include`, `fw/lvgl`, firmware docs)
2. Hardware source changes (`hw/...` project/schematic/pcb/library files)
3. Documentation-only changes

Example commit messages:
- `feat(fw): adjust fan auto hysteresis for smoother transitions`
- `feat(hw): update WallBaseBoard relay routing`
- `docs: update system architecture and pin mapping`

## First Steps

1. Open `fw/` in VS Code for firmware development.
2. Use PlatformIO tasks for build/upload.
3. Use root repository for cross-domain commits (fw + hw).

## License

MIT License. See `fw/LICENSE`.
