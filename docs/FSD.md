# FUNKCIONALNA SPECIFIKACIJA PROJEKTA (FSD) — Glavni Arhitektonski Dokument
**Verzija:** 4.0 (I2C Expander Arhitektura)
**Hardver:** ESP32-S3-WROOM-1-N8 (4848S040), 4.0" ST7701 Display, GT911 Touch, Custom PCB, I2C GPIO Expander
**Framework:** Arduino/PlatformIO (ESP-IDF Core), LVGL 8.3.11, SquareLine Studio
**I2C Expander:** PCF8574AN (testna faza) → PCA9554ADH (finalna/produkcijska faza)

---

## 1. HARDVERSKA KONFIGURACIJA (PINOUT)

### 1.1 ESP32-S3 Direktni Pinovi

| Pin | Funkcija | Tip | Napomena |
|-----|----------|-----|----------|
| **IO1** | NTC 100k B3950 Termistor | ADC1_CH0 (Analog Input) | Serijski otpornik 100kΩ prema 3.3V |
| **IO2** | I2C1 SDA → GPIO Expander | I2C Bidirekcionalni | Pull-up 4.7kΩ na 3.3V |
| **IO19** | I2C0 SDA → GT911 Touch | I2C Bidirekcionalni | Pull-up 4.7kΩ na 3.3V |
| **IO38** | LCD Backlight | PWM Output | 0-100% duty cycle |
| **IO40** | I2C1 SCL → GPIO Expander | I2C Output | Pull-up 4.7kΩ na 3.3V |
| **IO41** | RS485 DE (THVD1500) | Digital Output | Driver Enable — dedicated pin |
| **IO42** | REZERVA | — | Slobodan za buduća proširenja |
| **IO45** | I2C0 SCL → GT911 Touch | I2C Output | Pull-up 4.7kΩ na 3.3V |
| **IO48** | REZERVA | — | Slobodan za buduća proširenja |

### 1.2 I2C GPIO Expander (PCF8574AN / PCA9554ADH)

| Expander Pin | Funkcija | Smjer | I2C Adresa |
|--------------|----------|-------|------------|
| **P0** | Relay 1 (Grijanje/Hlađenje ili Brzina 1) | Output | PCF8574AN: 0x38 |
| **P1** | Relay 2 (Brzina 2) | Output | PCA9554ADH: 0x20 |
| **P2** | Relay 3 (Brzina 3) | Output | A0=A1=A2=GND |
| **P3** | Rezerva Output (ULN2003) | Output | — |
| **P4** | Rezerva Output (ULN2003) | Output | — |
| **P5** | Rezerva Output (ULN2003) | Output | — |
| **P6** | Senzor Prozora (Active LOW) | Input | Pull-up 10kΩ na 3.3V |
| **P7** | Rezerva Input | Input | — |

### 1.3 Dual I2C Bus Arhitektura

| Bus | Kontroler | Pinovi | Uređaj | Brzina |
|-----|-----------|--------|--------|--------|
| **I2C0** (`Wire`) | I2C0 hardverski | IO19 (SDA) / IO45 (SCL) | GT911 Touch Controller | 100 kHz |
| **I2C1** (`TwoWire(1)`) | I2C1 hardverski | IO2 (SDA) / IO40 (SCL) | GPIO Expander | 400 kHz |

### 1.4 Hardverska Zaštita — Obavezna na PCB-u

| Lokacija | Otpornik | Spoj | Svrha |
|----------|----------|------|-------|
| Exp P0 | 10kΩ | Pin → GND | Pull-down za Relay 1 (glitch zaštita) |
| Exp P1 | 10kΩ | Pin → GND | Pull-down za Relay 2 (glitch zaštita) |
| Exp P2 | 10kΩ | Pin → GND | Pull-down za Relay 3 (glitch zaštita) |
| Exp P3 | 10kΩ | Pin → GND | Pull-down za rezervni output (ULN2003) |
| Exp P4 | 10kΩ | Pin → GND | Pull-down za rezervni output (ULN2003) |
| Exp P5 | 10kΩ | Pin → GND | Pull-down za rezervni output (ULN2003) |
| Exp P6 | 10kΩ | Pin → 3.3V | Pull-up za Window Sensor |
| I2C1 SDA | 4.7kΩ | Pin → 3.3V | I2C pull-up |
| I2C1 SCL | 4.7kΩ | Pin → 3.3V | I2C pull-up |
| Exp VDD | 100nF | VDD ↔ GND | Decoupling kondenzator |

### 1.5 Napomene o Ekspanderima

**PCF8574AN (Testna Faza):**
- ⚠️ **NIJE glitch-free** — pri power-on pinovi su HIGH (~100kΩ internim pull-up)
- Open-drain izlazi sa inverznom logikom: `0 = relej ON`, `1 = relej OFF`
- Hardverski 10kΩ pull-down na P0-P2 je **OBAVEZAN** za zaštitu od boot glitch-a
- Max I2C brzina: 100 kHz (Standard mode)
- I2C adresa: `0x38` (A0=A1=A2=GND, "AN" varijanta)

**PCA9554ADH (Finalna/Produkcijska Faza):**
- ✅ **Glitch-free** — pri power-on svi pinovi su INPUT (high-Z), output latch = 0x00
- Push-pull izlazi sa direktnom logikom: `1 = relej ON`, `0 = relej OFF`
- Glitch-free inicijalizacija: (1) Postavi output latch na 0 → (2) Aktiviraj output smjer
- Max I2C brzina: 400 kHz (Fast mode)
- I2C adresa: `0x20` (A0=A1=A2=GND)

---

## 2. MODBUS RTU KOMUNIKACIJA

- **Protokol:** Modbus RTU Slave, 115200 8N1 (dijeli UART0 sa debug Serial).
- **RS485 Transceiver:** THVD1500, Driver Enable na **IO41** (dedicated pin, bez sharinga).
- **Adresa:** Podesiva (1-247), čuva se u NVS memoriji (default: 1).
- **Broadcast (ID 0):** Podrška za sinhronizaciju vremena i prognoze (bez odgovora).
- **Ključni Registri:**
    - `40001`: Target Temp (x10) — sinhronizovano sa `ui_ArcTemp` (10-40°C).
    - `40002`: HVAC Mode (0=OFF, 1=HEAT, 2=COOL).
    - `40022`: Relay Mode (0=3-Brzine, 1=1-Relej).
    - `40030-40046`: Weather Data (Prognoza za 5 dana).

---

## 3. LOGIKA SISTEMA I SIGURNOST

1. **Relejni Deadband (100ms):** Obavezan delay pri prebacivanju brzina (Svi OFF → 100ms → Ciljani ON).
2. **Window Sensor Priority:** Ako je prozor otvoren (Expander P6 = LOW), HVAC ide u OFF. GUI prikazuje upozorenje.
3. **Weather Watchdog:** Ako registar `40030` nije ažuriran >12h, Tile 3 (Weather) se sakriva iz `ui_SwipeContainer`.
4. **Inactivity Timeout:** Ako je sistem na bilo kojem `Settings` ekranu, nakon 30s bez touch-a, vraća se na `ui_Main`. Izuzetak je aktiviran WiFi Manager.
5. **Clean Mode:** Blokira sav touch input na 60s (koristi `lv_layer_sys()`).
6. **Boot-time Relej Zaštita:**
   - **PCF8574AN:** Hardverski pull-down otpornici (10kΩ) osiguravaju da releji ostanu OFF tokom I2C inicijalizacije.
   - **PCA9554ADH:** Softverska glitch-free sekvenca — output latch se postavlja na 0 PRIJE aktivacije output pinova.
   - Svi releji su u OFF stanju unutar ~420ms od power-on-a.

---

## 4. KORISNIČKI INTERFEJS (GUI)

Glavna navigacija koristi **ui_SwipeContainer** (lv_tileview) sa horizontalnim panelima:

### 4.1. Tile 1: Main (Home)
- Prikaz sata, unutrašnje/vanjske temperature.
- Dugme `ui_ButtonGoToThermostat` vodi direktno na Tile 2.
- Skriveni meni: Long press (5s) na `ui_ButtonHiddenMenu` (gornji lijevi ugao) otvara `ui_PinEntry`.

### 4.2. Tile 2: Thermostat
- **Setpoint:** Kontrola preko `ui_ArcTemp` i tastera `ui_BtnTempInc`/`ui_BtnTempDec` (Opseg: 10-40°C).
- **Fan Speed:** Ciklična promjena (Auto → Low → Mid → High) preko `ui_BtnFan`.

### 4.3. Tile 3: Weather
- Prikaz 5-dnevne prognoze koristeći 5 statičkih kartica (`CardDay1` do `CardDay5`).
- Svaka kartica sadrži: Dan, Ikonu, Maksimalnu (High) i Minimalnu (Low) temperaturu.
- Vizuelni indikatori: Crvena boja za High temp, plava za Low temp.

### 4.4. Tile 4: Clean Screen
- Aktivacija preko `ui_BtnCleanStart`. Blokada unosa na sistemskom nivou.

---

## 5. POSTAVKE I ODRŽAVANJE (SERVICE MENU)

Pristup putem PIN-a: **1234**.

### 5.1. Arhitektura Podešavanja
- **Dirty Flag Bitmask:** Sve promjene u UI-ju se čuvaju u RAM-u i markiraju u 32-bitnoj maski (`settings_flag_t`).
- **NVS Upis:** Upis u memoriju se vrši samo pri pritisku na "Save & Exit" i to samo za izmijenjene parametre.

### 5.2. Podjela Settings ekrana:
- **Settings 1:** Min/Max Temp, HVAC Mode, Control Type.
- **Settings 2:** Hysteresis, Stage Step, Sensor Offset (Spinbox).
- **Settings 3:** Brightness (High/Low Slider), Timeout, Modbus ID, WiFi Manager Switch.

---

## 6. WI-FI I ODRŽAVANJE

- **WiFi Manager (AP mod):** Aktivira se preko `ui_SwitchStartAp` u Settings 3.
- Dok je AP aktivan, **Inactivity Timeout** je pauziran.
- Ažuriranje firmware-a se vrši isključivo u ovom modu.

---

## 7. SOFTVERSKA ARHITEKTURA — I2C EXPANDER ADAPTER SLOJ

### 7.1 Adapter API (`hal.h` / `hal.cpp`)

Sav pristup relejima i window senzoru ide isključivo kroz adapter funkcije. Direktni `digitalWrite()` / `digitalRead()` pozivi za ove pinove su **uklonjeni** iz `hvac.cpp` i `modbus_handler.cpp`.

| Funkcija | Opis | Parametri |
|----------|------|-----------|
| `hal_relay_set(id, on)` | Postavlja stanje releja | `id`: 1,2,3 · `on`: true/false |
| `hal_relay_is_on(id)` | Provjerava da li je relej aktivan | `id`: 1,2,3 → `bool` |
| `hal_relay_all_off()` | Gasí sve releje odjednom | — |
| `hal_window_is_open()` | Čita stanje window senzora | → `bool` (true = otvoren) |
| `hal_expander_read_inputs()` | Čita sve input pinove expandera | → `uint8_t` (raw byte) |

### 7.2 Konfiguracija Tipa Expandere

Promjena tipa expandere se vrši **jednom linijom** u `include/hal.h`:

```c
// Testna faza (PCF8574AN — NIJE glitch-free)
#define EXPANDER_TYPE  EXPANDER_TYPE_PCF8574

// Finalna faza (PCA9554ADH — glitch-free)
#define EXPANDER_TYPE  EXPANDER_TYPE_PCA9554
```

Adapter API automatski prilagođava:
- I2C adresu (0x38 vs 0x20)
- Logiku izlaza (inverzna vs direktna)
- Inicijalizacijsku sekvencu (jednostavna vs glitch-free)
- Register mapu (jedan register vs 4 registra)

### 7.3 Boot Sekvenca (Timeline)

```
0ms      — Power-on / Reset
0-50ms   — ROM bootloader (strapping pins: GPIO0,3,45,46)
50-200ms — 2nd stage bootloader
200-400ms — Arduino framework init, app_main()
400ms    — setup() → board_hal_init()
400-410ms — I2C1 init (IO2/IO40, 400kHz)
410-420ms — Expander init (releji postavljeni na OFF)
420ms    — RS485 DE init (IO41 → LOW, receive mode)
420-430ms — Backlight init (~50%)
430-550ms — Display init (gfx->begin(), ST7701 SPI)
550ms+   — LVGL init, Touch init, Modbus init, HVAC init
```
