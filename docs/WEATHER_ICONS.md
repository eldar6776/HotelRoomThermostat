# Weather Icon Mapping Reference

## Overview
Weather forecast display uses icon IDs to represent different weather conditions. This document explains the current mapping and how to add new weather icons.

---

## Current Icon Mapping

### Firmware Definitions (`include/modbus_handler.h`)
```c
#define WX_ICON_SUNNY       0   // Clear sky / Sunny day
#define WX_ICON_PARTLY_CLR  1   // Partly cloudy / Sunny with clouds  
#define WX_ICON_CLOUDY      2   // Overcast / Cloudy
#define WX_ICON_RAINY       3   // Rain
#define WX_ICON_SNOWY       4   // Snow
#define WX_ICON_STORMY      5   // Thunderstorm
#define WX_ICON_FOGGY       6   // Fog / Mist
#define WX_ICON_WINDY       7   // Windy conditions
```

### UI Icon Mapping (`src/ui_events.c`)
| Icon ID | Constant | LVGL Asset | Status |
|---------|----------|------------|--------|
| 0 | WX_ICON_SUNNY | `ui_img_sunny_png` | ✅ Available |
| 1 | WX_ICON_PARTLY_CLR | `ui_img_sunny_day_png` | ✅ Available |
| 2 | WX_ICON_CLOUDY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |
| 3 | WX_ICON_RAINY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |
| 4 | WX_ICON_SNOWY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |
| 5 | WX_ICON_STORMY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |
| 6 | WX_ICON_FOGGY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |
| 7 | WX_ICON_WINDY | `ui_img_hvac_png` | ⚠️ Fallback (needs proper icon) |

---

## How to Add New Weather Icons

### Step 1: Prepare Icon Assets
1. Create PNG icons (recommend 64×64 or 80×80 pixels)
2. Name them descriptively:
   - `cloudy.png` - for overcast conditions
   - `rainy.png` - for rain
   - `snowy.png` - for snow
   - `stormy.png` - for thunderstorms
   - `foggy.png` - for fog/mist
   - `windy.png` - for windy conditions

### Step 2: Import to SquareLine Studio
1. Open project in SquareLine Studio
2. Go to **Assets** panel
3. Click **Add Image**
4. Select your PNG files
5. Name them consistently: `ui_img_cloudy_png`, `ui_img_rainy_png`, etc.
6. Export project (generates C files in `lvgl/` folder)

### Step 3: Update Firmware Code

#### 3.1 Add External Declarations (`src/ui_events.c`)
```c
// Forward declarations for PNG images (weather forecast icons)
extern const lv_img_dsc_t ui_img_sunny_png;      // WX_ICON_SUNNY (0)
extern const lv_img_dsc_t ui_img_sunny_day_png;  // WX_ICON_PARTLY_CLR (1)
extern const lv_img_dsc_t ui_img_cloudy_png;     // WX_ICON_CLOUDY (2) ← ADD THIS
extern const lv_img_dsc_t ui_img_rainy_png;      // WX_ICON_RAINY (3) ← ADD THIS
extern const lv_img_dsc_t ui_img_snowy_png;      // WX_ICON_SNOWY (4) ← ADD THIS
extern const lv_img_dsc_t ui_img_stormy_png;     // WX_ICON_STORMY (5) ← ADD THIS
extern const lv_img_dsc_t ui_img_foggy_png;      // WX_ICON_FOGGY (6) ← ADD THIS
extern const lv_img_dsc_t ui_img_windy_png;      // WX_ICON_WINDY (7) ← ADD THIS
extern const lv_img_dsc_t ui_img_hvac_png;       // Generic fallback icon
```

#### 3.2 Update Icon Lookup Function (`src/ui_events.c`)
```c
static const lv_img_dsc_t *weather_icon_src(uint8_t icon_id)
{
    switch (icon_id) {
        case WX_ICON_SUNNY:       // 0 - Clear sky
            return &ui_img_sunny_png;
        case WX_ICON_PARTLY_CLR:  // 1 - Partly cloudy
            return &ui_img_sunny_day_png;
        case WX_ICON_CLOUDY:      // 2 - Cloudy
            return &ui_img_cloudy_png;     // ← REPLACE FALLBACK
        case WX_ICON_RAINY:       // 3 - Rain
            return &ui_img_rainy_png;      // ← REPLACE FALLBACK
        case WX_ICON_SNOWY:       // 4 - Snow
            return &ui_img_snowy_png;      // ← REPLACE FALLBACK
        case WX_ICON_STORMY:      // 5 - Storm
            return &ui_img_stormy_png;     // ← REPLACE FALLBACK
        case WX_ICON_FOGGY:       // 6 - Fog
            return &ui_img_foggy_png;      // ← REPLACE FALLBACK
        case WX_ICON_WINDY:       // 7 - Wind
            return &ui_img_windy_png;      // ← REPLACE FALLBACK
        default:
            return &ui_img_hvac_png;  // Generic fallback
    }
}
```

### Step 4: Test Icons
```bash
cd test
python modbus_test.py --cycles 3
```

Test script will send varying weather icon IDs (0-7). Check display to verify icons appear correctly.

---

## Modbus Protocol

### Weather Data Structure
Each day occupies 3 Modbus holding registers (40031-40045 for 5 days):

| Register | Field | Description |
|----------|-------|-------------|
| Base + 0 | `day_id \| (icon_id << 8)` | Lower byte = day (0-6 = Sun-Sat)<br/>Upper byte = icon ID (0-7) |
| Base + 1 | `temp_high_c10` | High temperature × 10 (e.g., 250 = 25.0°C) |
| Base + 2 | `temp_low_c10` | Low temperature × 10 (e.g., 180 = 18.0°C) |

### Example: Write Weather Data
```python
# Day 0 (Monday): Cloudy icon (2), High 24°C, Low 18°C
day_id = 1       # Monday
icon_id = 2      # Cloudy
packed = day_id | (icon_id << 8)  # = 0x0201 = 513

client.write_register(address=30, value=packed)       # Day + Icon
client.write_register(address=31, value=240)          # 24.0°C × 10
client.write_register(address=32, value=180)          # 18.0°C × 10

# Trigger watchdog to parse data
client.write_register(address=29, value=1)            # MB_REG_WX_WATCHDOG
```

---

## Test Script Icon Names

Update `test/modbus_test.py` icon_names dictionary when adding new assets:

```python
icon_names = {
    WX_ICON_SUNNY: "Sunny",
    WX_ICON_PARTLY_CLR: "Partly Cloudy",
    WX_ICON_CLOUDY: "Cloudy",           # ← UPDATE DESCRIPTION
    WX_ICON_RAINY: "Rainy",             # ← UPDATE DESCRIPTION
    WX_ICON_SNOWY: "Snowy",             # ← UPDATE DESCRIPTION
    WX_ICON_STORMY: "Stormy",           # ← UPDATE DESCRIPTION
    WX_ICON_FOGGY: "Foggy",             # ← UPDATE DESCRIPTION
    WX_ICON_WINDY: "Windy"              # ← UPDATE DESCRIPTION
}
```

---

## Design Guidelines

### Icon Specifications
- **Size**: 64×64 or 80×80 pixels (match existing icons)
- **Format**: PNG with transparency
- **Style**: Simple, clear, easily recognizable at small sizes
- **Color**: Use consistent color palette with UI theme
- **Background**: Transparent

### Recommended Icon Sources
- [Weather Icons](https://erikflowers.github.io/weather-icons/) - Open source weather icon set
- [Flaticon Weather](https://www.flaticon.com/packs/weather-163) - Free with attribution
- Custom design matching UI aesthetic

---

## Troubleshooting

### Icons Not Displaying
1. **Verify export**: Check `lvgl/` folder for `ui_img_<name>_png.c` files
2. **Check declarations**: Ensure `extern const lv_img_dsc_t` added to `ui_events.c`
3. **Rebuild firmware**: Clean build with `platformio run --target clean`
4. **Flash device**: `platformio run --target upload`

### Wrong Icon Displayed
1. **Check icon_id**: Verify test script sends correct icon ID (0-7)
2. **Debug mapping**: Add `LOG_INFO` in `weather_icon_src()` to log icon_id
3. **Check switch case**: Ensure icon_id matches case in switch statement

---

## Version History
- **v1.0** (2026-04-01): Initial mapping with 2 icons (sunny, sunny_day)
- **v1.1** (2026-04-01): Expanded to 8 weather types with fallback support
