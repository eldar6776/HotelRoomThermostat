# TASK 02: Inicijalizacija Displeja, Touch-a i Pinova
**Cilj:** Postaviti hardverski sloj (HAL) za 4.0" ST7701 panel i GT911 touch.

**Upute:**
1.  **Ekran (ST7701 16-bit RGB):**
    - Konfigurisati RGB panel (Bus, Clock, Vsync/Hsync parametre za 480x480).
    - Inicijalizovati LVGL buffer u PSRAM-u za maksimalne performanse.
2.  **Touch (GT911):**
    - SDA: IO19, SCL: IO20, Adresa: 0x5D.
    - Napisati funkciju za `lv_indev_drv_t` koja vraća tačne koordinate.
3.  **Backlight (PWM):**
    - IO38, rezolucija 10-bit (0-1023) ili 8-bit (0-255).
    - Inicijalizovati na defaultnu vrijednost iz NVS-a.
4.  **Senzori i Releji:**
    - Releji (IO40, IO2, IO39) kao `OUTPUT` (default LOW).
    - Window Sensor (IO41) kao `INPUT_PULLUP`.
    - NTC (IO1) ADC inicijalizacija (ADC1_CH0).

**Očekivani rezultat:** Stabilna slika na ekranu, funkcionalan dodir i ispravno očitavanje stanja pina IO41 na serijski monitor.
