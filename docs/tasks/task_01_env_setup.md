# TASK 01: Okruženje, Particije i Biblioteke
**Cilj:** Postaviti stabilnu osnovu za ESP32-S3 projekat sa 16MB Flash i 8MB PSRAM.

**Upute:**
1.  **platformio.ini:**
    - Board: `esp32-s3-devkitc-1` (ili specifični 4848S040 profil).
    - Framework: `arduino`.
    - Omogućiti PSRAM (`-DBOARD_HAS_PSRAM`, `mspi` config).
    - Brzina uploada: `921600`.
2.  **Particije (`partitions.csv`):**
    - Kreirati `huge_app` profil: 16MB Flash, bar 4-8MB za aplikaciju (OTA se ne koristi primarno, ali ostaviti prostora za SquareLine asete).
3.  **Biblioteke:**
    - `lvgl@8.3.x`
    - `Arduino_GFX` (ili sličan drajver za ST7701 RGB ekran)
    - `Modbus-esp8266` (odlična podrška za RTU Slave na ESP32)
    - `ArduinoJson` (opcionalno za buduće proširenja)
4.  **Struktura Foldera:**
    - `/src`, `/lib`, `/ui` (za SquareLine izvoz).

**Očekivani rezultat:** Projekt koji se kompajlira sa osnovnim "Hello World" na serijskom portu i prepoznaje sav raspoloživi PSRAM.
