# Gemini Project Analysis & Cheat Sheet
**Projekat:** Hotel Room Thermostat
**Hardver:** ESP32-S3 (4848S040), 4.0" ST7701 LCD, GT911 Touch, Custom PCB
**Verzija:** 1.0.0 (Production Ready)

Ovaj dokument služi kao brzi referentni vodič i sistemski pregled arhitekture za AI asistenta (Gemini) tokom daljeg razvoja.

---

## 1. Sistemska Arhitektura (Ključni Moduli)

| Modul | Fajlovi | Odgovornost | Kritična Pravila |
|-------|---------|-------------|------------------|
| **HAL** | `hal.h`, `hal.cpp` | Inicijalizacija hardvera (Displej, LVGL draw buffer, Touch, I2C1 Expander, Backlight, RS485 DE). | Svi releji i Window senzor idu isključivo preko `hal_relay_*` i `hal_window_is_open()`. Nema direktnog `digitalWrite()`. |
| **HVAC** | `hvac.h`, `hvac.cpp` | Čitanje NTC senzora (EMA filter), Termostatska logika (Histereza), Deadband. | Održavanje 100ms mrtvog vremena pri prebacivanju releja. Prozor otvoren = HVAC OFF. |
| **Modbus** | `modbus_handler.*`| RS485 RTU Slave komunikacija na 115200 baud, 8N1. Mapiranje registara. | **ZABRANJENO** korištenje `Serial.print()` ili `LOG_*` unutar `cb_hreg_write` zbog dijeljenog UART0 pina! |
| **UI** | `ui_events.c`, `main.cpp` | Nezavisni ekrani (Main, Thermostat, Clean), Arc kontrola, interakcija korisnika, DND/MUR logika. | Korištenje `g_dirty_flags` u `settings.cpp` za batch upis u NVS pri izlasku iz postavki. |

---

## 2. Memorija i Hardverski Resursi

*   **Flash / RAM:** 16MB Flash, 8MB OPI PSRAM (`huge_app` particija, `qio_opi`).
*   **UART0 (IO43/IO44):** Dijeljen između Serial monitora i Modbus RTU. Strogo kontrolisan tajming.
*   **I2C0 (IO19/IO45 - 100kHz):** Samo za GT911 Touch.
*   **I2C1 (IO2/IO40 - 400kHz):** Samo za GPIO Expander (PCF8574AN ili PCA9554ADH).
*   **ADC (IO1):** NTC Termistor (100k B3950).

---

## 3. Stanje I2C Expandera (Trenutni Fokus)

Aktivna je tranzicija sa **PCF8574AN** (testna faza, ima boot glitch) na **PCA9554ADH** (produkcija, glitch-free).
*   **Konfiguracija:** Kontroliše se preko `#define EXPANDER_TYPE` u `hal.h`.
*   **Releji:** P0 (Relay 1), P1 (Relay 2), P2 (Relay 3). P6 (Window Sensor - Active LOW).
*   **Logika rada:** HAL adapter `hal.cpp` automatski prevodi inverznu logiku PCF-a i direktnu logiku PCA expandera.

---

## 4. Modbus Register Map (Brzi Podsjetnik)

**Holding Registri (40001+ / R/W):**
*   `40001` (0): Target Temp (x10)
*   `40002` (1): HVAC Mode (0=OFF, 1=HEAT, 2=COOL)
*   `40003` (2): Fan Speed (0=AUTO, 1=LOW, 2=MID, 3=HIGH)
*   `40022` (21): Relay Mode (0=3-Speed, 1=1-Relay)
*   `40030` (29): Weather Watchdog
*   `40031-40045`: Weather Data (5 dana)

**Input Registri (30001+ / Read Only):**
*   `30001` (0): Current Temp (x10)
*   `30003` (2): Relay Status (bit0=R1, bit1=R2, bit2=R3)
*   `30007` (6): Window Raw (0=open, 1=closed)

**Coils (00001+):**
*   `00001`: DND (Do Not Disturb)
*   `00002`: MUR (Make Up Room)

---

## 5. Razvojni Ciljevi i Roadmap (Šta je sljedeće?)

Prema FSD-u i README.md dokumentaciji, prioritetne naredne stavke (v1.1.0 / v1.2.0) bi mogle biti:
1.  **Hardware tranzicija:** Finalno testiranje PCA9554ADH (fizičko lemljenje i verifikacija koda koji je već napisan).
2.  **Senzor Validacija:** Rješavanje anomalije čitanja (6500+°C) kada je NTC isključen sa ploče.
3.  **Auto-save Modbus:** Čuvanje promjene adrese bez potrebe za "Save & Exit" klikom.
4.  **UI Unapređenja:** Touch kalibracioni ekran i lokalizacija (i18n).

---
*Dokument automatski održava i konsultuje AI asistent pri svakom tehničkom zadatku.*