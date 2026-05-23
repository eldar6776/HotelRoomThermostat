# Hotel Room Thermostat

Repozitorij objedinjuje kompletan projekat sobnog termostata za hotelske sobe:
- **fw/** – firmware za ESP32-S3 termostat sa touchscreen interfejsom
- **hw/** – hardverski dizajn prilagođene zidne/base board ploče
- **sw/** – SquareLine Studio projekat i UI resursi korišteni za generisanje LVGL ekrana

Ovaj README opisuje trenutno stanje repozitorija i služi kao ulazna tačka za razvoj firmware-a, UI-ja i hardvera.

## Struktura repozitorija

- `fw/` PlatformIO firmware projekat
- `hw/` Altium Designer projekat za hardver
- `sw/` SquareLine Studio projekat za UI dizajn i generisanje ekrana

## Trenutno stanje projekta

Projekat trenutno sadrži tri aktivna dijela:

### 1. Firmware (`fw/`)
Firmware cilja **ESP32-S3 4848S040** platformu i koristi **Arduino framework** kroz PlatformIO.

Glavne funkcionalnosti koje su prisutne u kodu:
- LVGL grafički interfejs za 4.0" ST7701 displej sa GT911 touch kontrolerom
- Modbus RTU slave komunikacija
- HVAC logika za grijanje/hlađenje i upravljanje ventilatorom
- DND i MUR kontrole u interfejsu
- Čuvanje podešavanja u NVS
- LittleFS podrška za učitavanje prilagođenog logotipa
- Wi‑Fi konfiguracija i periodična NTP sinhronizacija vremena
- Prikaz vanjske temperature kroz Modbus registre
- Automatsko uvećavanje build verzije preko `version_increment.py`

Važni firmware fajlovi:
- `fw/platformio.ini`
- `fw/src/main.cpp`
- `fw/docs/FSD.md`
- `fw/docs/DIAGRAMS.md`
- `fw/docs/hvac_diagnostics_checklist.md`

### 2. Hardver (`hw/`)
Hardverski dio je organizovan u projektu:
- `hw/WallBaseBoard/`

U tom folderu se trenutno nalaze glavni Altium fajlovi:
- `WallBaseBoard.PrjPcb`
- `WallBaseBoard.SchDoc`
- `WallBaseBoard.PcbDoc`
- biblioteke simbola i footprinta (`.SCHLIB`, `.PcbLib`)
- izlazni i pomoćni fajlovi kao što su `.OutJob` i PDF export

### 3. UI / SquareLine projekat (`sw/`)
`sw/` više nije samo rezervisan za pomoćne alate, nego sadrži aktivni **SquareLine Studio** projekat:
- `HotelRoomThermostat.spj`
- `HotelRoomThermostat.slp`
- `HotelRoomThermostat.sll`
- `HotelRoomThermostat_events.py`
- `assets/`, `boards/`, `backup/`
- `project.info`

Prema `sw/project.info`, projekat je editovan u **SquareLine Studio 1.6.0**.

## Firmware build i upload

Iz `fw/` direktorija:

```bash
platformio run
platformio run --target upload
platformio device monitor
```

Podrazumijevani PlatformIO environment je:
- `thermostat`

Ključne biblioteke definisane u `fw/platformio.ini`:
- `lvgl/lvgl`
- `moononournation/GFX Library for Arduino`
- `emelianov/modbus-esp8266`
- `bblanchon/ArduinoJson`
- `tzapu/WiFiManager`

## Radni tok po komponentama

### Firmware razvoj
1. Otvori `fw/` u VS Code / PlatformIO okruženju.
2. Buildaj i flashaj firmware na ESP32-S3 uređaj.
3. Za serijski log koristi `platformio device monitor`.

### UI izmjene
1. Otvori `sw/HotelRoomThermostat.spj` u SquareLine Studio.
2. Uredi ekrane, teme i assete.
3. Generisane LVGL fajlove uskladi sa sadržajem u `fw/lvgl/`.

### Hardverske izmjene
1. Otvori `hw/WallBaseBoard/WallBaseBoard.PrjPcb` u Altium Designer-u.
2. Uredi šemu, PCB i biblioteke po potrebi.
3. Po potrebi regeneriši proizvodne izlaze i PDF dokumentaciju.

## Dokumentacija

Dodatna dokumentacija za firmware nalazi se u:
- `fw/docs/FSD.md`
- `fw/docs/DIAGRAMS.md`
- `fw/docs/gemini_analysis.md`
- `fw/docs/hvac_diagnostics_checklist.md`

## Preporuka za commit poruke

Za pregledniju historiju commita preporučeno je razdvojiti izmjene po domenima:
- `feat(fw): ...` za firmware
- `feat(hw): ...` za hardver
- `feat(sw): ...` za SquareLine/UI projekat
- `docs: ...` za dokumentaciju

Primjeri:
- `feat(fw): add ntp sync fallback logic`
- `feat(sw): update thermostat screen layout`
- `feat(hw): adjust WallBaseBoard relay routing`
- `docs: refresh root readme to match repository state`

## Prvi koraci

1. Ako radiš firmware, kreni iz foldera `fw/`.
2. Ako radiš UI, koristi `sw/` i SquareLine Studio projekat.
3. Ako radiš elektroniku, koristi `hw/WallBaseBoard/`.
4. Za izmjene koje zahvataju više domena koristi root repozitorij kao glavnu tačku rada.

## Licenca

MIT License. Pogledati `fw/LICENSE` ako je prisutan u firmware dijelu projekta.
