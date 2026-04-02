# TASK 02: Inicijalizacija Displeja, Touch-a i I2C GPIO Expandera
**Cilj:** Postaviti hardverski sloj (HAL) za 4.0" ST7701 panel, GT911 touch i I2C GPIO expander (PCF8574AN za test, PCA9554ADH za produkciju).

**Upute:**
1.  **Ekran (ST7701 16-bit RGB):**
    - Konfigurisati RGB panel (Bus, Clock, Vsync/Hsync parametre za 480x480).
    - Inicijalizovati LVGL buffer u PSRAM-u za maksimalne performanse.
2.  **Touch (GT911 — I2C0):**
    - SDA: IO19, SCL: IO45, Adresa: 0x5D.
    - Napisati funkciju za `lv_indev_drv_t` koja vraća tačne koordinate.
    - Koristi `Wire` (I2C0 hardverski kontroler).
3.  **I2C1 za GPIO Expander (I2C1 — nezavisni kontroler):**
    - SDA: IO2, SCL: IO40.
    - Kreirati `TwoWire s_extI2C(1)` — nezavisni hardverski I2C1 kontroler.
    - Brzina: 400 kHz (Fast mode).
    - Pull-up otpornici: 4.7kΩ na 3.3V za SDA i SCL.
    - NEMA konflikta sa GT911 (I2C0) — potpuno nezavisni kontroleri.
4.  **GPIO Expander Inicijalizacija:**
    - **PCF8574AN (testna faza):**
        - I2C adresa: 0x38 (A0=A1=A2=GND).
        - Postaviti sve izlaze na HIGH (releji OFF — open-drain inactive).
        - ⚠️ Hardverski 10kΩ pull-down na P0-P2 je OBAVEZAN za glitch zaštitu.
    - **PCA9554ADH (finalna faza):**
        - I2C adresa: 0x20 (A0=A1=A2=GND).
        - Glitch-free sekvenca: (1) Output latch = 0x00 → (2) Config register = 0xF0.
        - ✅ Nema glitch-a — releji su OFF od trenutka paljenja.
5.  **Adapter API (`hal.h` / `hal.cpp`):**
    - Implementirati `hal_relay_set(id, on)` — postavlja stanje releja (id: 1,2,3).
    - Implementirati `hal_relay_is_on(id)` — provjerava da li je relej aktivan.
    - Implementirati `hal_relay_all_off()` — gasi sve releje odjednom.
    - Implementirati `hal_window_is_open()` — čita Window Sensor sa expander P6 (Active LOW).
    - Implementirati `hal_expander_read_inputs()` — čita sve input pinove expandera.
6.  **RS485 DE Pin:**
    - IO41 kao `OUTPUT`, default LOW (receive mode).
    - THVD1500 Driver Enable — dedicated pin, bez sharinga.
7.  **Backlight (PWM):**
    - IO38, rezolucija 10-bit (0-1023) ili 8-bit (0-255).
    - Inicijalizovati na defaultnu vrijednost iz NVS-a.
8.  **NTC Termistor:**
    - IO1 (ADC1_CH0) — konfigurisati ADC sa 11db attenuacije (0-3.3V range).

**Očekivani rezultat:** Stabilna slika na ekranu, funkcionalan dodir, I2C expander inicijaliziran sa relejima u OFF stanju, RS485 DE pin spreman, adapter API funkcionalan.
