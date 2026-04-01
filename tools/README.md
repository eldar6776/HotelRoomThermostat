# Tools - Weather Icon Converter

## Quick Start

### Windows (Najbrži način)
```cmd
tools\convert_weather_icons.bat
```
Skripta će automatski:
1. Provjeriti Python instalaciju
2. Instalirati Pillow ako treba
3. Konvertovati SVE PNG ikone u C fajlove

---

## Manual Method

### Preduslov: Instaliraj Pillow
```bash
pip install Pillow
```

### Pokreni konverziju
```bash
python tools/weather_icon_converter.py
```

---

## Šta skripta radi?

### Input
Traži PNG fajlove u `lvgl/assets/`:
- ✅ cloudy.png
- ✅ foggy.png
- ✅ partly_cloudy_day.png
- ✅ partly_cloudy_night.png
- ✅ rainy.png
- ✅ snowy.png
- ✅ storm.png
- ✅ sunny_day.png
- ✅ sunny.png
- ✅ clear_night.png

### Output
Generiše C fajlove u `lvgl/`:
- `ui_img_cloudy_png.c`
- `ui_img_foggy_png.c`
- `ui_img_partly_cloudy_day_png.c`
- `ui_img_partly_cloudy_night_png.c`
- `ui_img_rainy_png.c`
- `ui_img_snowy_png.c`
- `ui_img_storm_png.c`
- `ui_img_sunny_day_png.c`
- `ui_img_sunny_png.c`
- `ui_img_clear_night_png.c`

### Format
- **Color Format**: ARGB8888 (True Color with Alpha)
- **Compatible**: LVGL 8.3+
- **Memory Aligned**: Da (embedded systems compatible)

---

## Nakon konverzije

### 1. Dodaj extern deklaracije (`src/ui_events.c`)

```c
// Forward declarations for PNG images (weather forecast icons)
extern const lv_img_dsc_t ui_img_sunny_png;             // WX_ICON_SUNNY (0)
extern const lv_img_dsc_t ui_img_sunny_day_png;         // WX_ICON_PARTLY_CLR (1)
extern const lv_img_dsc_t ui_img_cloudy_png;            // WX_ICON_CLOUDY (2)
extern const lv_img_dsc_t ui_img_rainy_png;             // WX_ICON_RAINY (3)
extern const lv_img_dsc_t ui_img_snowy_png;             // WX_ICON_SNOWY (4)
extern const lv_img_dsc_t ui_img_storm_png;             // WX_ICON_STORMY (5)
extern const lv_img_dsc_t ui_img_foggy_png;             // WX_ICON_FOGGY (6)
extern const lv_img_dsc_t ui_img_partly_cloudy_day_png; // Alternative for WX_ICON_WINDY (7)
extern const lv_img_dsc_t ui_img_hvac_png;              // Generic fallback icon
```

### 2. Ažuriraj mapiranje (`src/ui_events.c`)

```c
static const lv_img_dsc_t *weather_icon_src(uint8_t icon_id)
{
    switch (icon_id) {
        case WX_ICON_SUNNY:       // 0 - Clear sky
            return &ui_img_sunny_png;
        case WX_ICON_PARTLY_CLR:  // 1 - Partly cloudy / Sunny day
            return &ui_img_sunny_day_png;
        case WX_ICON_CLOUDY:      // 2 - Cloudy
            return &ui_img_cloudy_png;        // ✅ NOVO!
        case WX_ICON_RAINY:       // 3 - Rain
            return &ui_img_rainy_png;         // ✅ NOVO!
        case WX_ICON_SNOWY:       // 4 - Snow
            return &ui_img_snowy_png;         // ✅ NOVO!
        case WX_ICON_STORMY:      // 5 - Storm
            return &ui_img_storm_png;         // ✅ NOVO!
        case WX_ICON_FOGGY:       // 6 - Fog
            return &ui_img_foggy_png;         // ✅ NOVO!
        case WX_ICON_WINDY:       // 7 - Windy (using partly cloudy as placeholder)
            return &ui_img_partly_cloudy_day_png;  // ✅ NOVO!
        default:
            return &ui_img_hvac_png;  // Generic fallback
    }
}
```

### 3. Rebuild firmware
```bash
platformio run --target upload
```

### 4. Test
```bash
cd test
python modbus_test.py --cycles 3
```

---

## Troubleshooting

### ERROR: Python not found
Instaliraj Python 3.7+ sa https://www.python.org/

### ERROR: PIL/Pillow not found
```bash
pip install Pillow
```

### ERROR: Assets directory not found
Pokreni skriptu iz root direktorija projekta:
```bash
cd C:\ProjektiOtvoreni\HotelRoomThermostat
python tools\weather_icon_converter.py
```

### Conversion successful but icons not displaying
1. Provjeri da li su C fajlovi generisani u `lvgl/` folderu
2. Dodaj extern deklaracije u `src/ui_events.c`
3. Ažuriraj `weather_icon_src()` funkciju
4. Clean build: `platformio run --target clean`
5. Upload: `platformio run --target upload`

---

## Alternativni metod: lv_img_conv (Node.js)

Ako želiš koristiti zvaničan LVGL tool:

### Instalacija
```bash
npm install -g lv_img_conv
```

### Konverzija pojedinačnog fajla
```bash
lv_img_conv lvgl/assets/cloudy.png -f CF_TRUE_COLOR_ALPHA -o lvgl/ui_img_cloudy_png.c
```

### Batch konverzija (PowerShell)
```powershell
Get-ChildItem lvgl\assets\*.png | ForEach-Object {
    $name = $_.BaseName.Replace("-", "_")
    lv_img_conv $_.FullName -f CF_TRUE_COLOR_ALPHA -o "lvgl\ui_img_${name}_png.c"
}
```

---

## File Size Optimization

PNG ikone su compressed, ali C array je raw data. Za optimizaciju:

### 1. Smanji rezoluciju PNG-a (prije konverzije)
```bash
# Resize to 48x48 (from 64x64)
mogrify -resize 48x48 lvgl/assets/*.png
```

### 2. Koristi 16-bit color umjesto 32-bit
Promijeni u skripti:
```python
LVGL_COLOR_FORMAT = "CF_TRUE_COLOR"  # 16-bit RGB565 (bez alpha)
```

### 3. Koristi RLE compression (LVGL feature)
```python
LVGL_COLOR_FORMAT = "CF_TRUE_COLOR_ALPHA_RLE"  # Compressed
```

---

## Icon Sources

Trenutne ikone (ako trebaš replacement):
- **Source**: Free weather icons (MIT License)
- **Size**: 64×64 pixels
- **Format**: PNG with transparency
- **Style**: Minimalist, flat design

Alternative icon packs:
- [Weather Icons](https://erikflowers.github.io/weather-icons/) - Open source
- [Flaticon Weather](https://www.flaticon.com/packs/weather-163) - Free with attribution
- [Feather Icons](https://feathericons.com/) - MIT License

---

## Version History
- **v1.0** (2026-04-01): Initial batch converter script
  - Manual PNG → C conversion using PIL/Pillow
  - Support for lv_img_conv tool
  - Windows batch wrapper (.bat file)
