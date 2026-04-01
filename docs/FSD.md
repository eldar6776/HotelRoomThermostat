# FUNKCIONALNA SPECIFIKACIJA PROJEKTA (FSD) - Glavni Arhitektonski Dokument
**Verzija:** 3.0 (Ažurirano na osnovu UI Event Logic v1.0)
**Hardver:** ESP32-S3 (4848S040), 4.0" ST7701 Display, GT911 Touch, Custom PCB.
**Framework:** Arduino/PlatformIO (ESP-IDF Core), LVGL 8.3.11, SquareLine Studio.

---

## 1. HARDVERSKA KONFIGURACIJA (PINOUT)
Ovi pinovi su fiksni i ne smiju se mijenjati:
- **IO40:** Relej 1 (Grijanje/Hlađenje ili Brzina 1)
- **IO2:** Relej 2 (Brzina 2)
- **IO39:** Relej 3 (Brzina 3)
- **IO1:** Analogni ulaz (ADC1_CH0) - NTC Termistor 100k (B3950)
- **IO41:** Digitalni ulaz - Senzor Prozora (Active Low)
- **IO48:** RS485 RTS (Transmit Enable za THVD1500)
- **IO38:** LCD Backlight (PWM kontrola, 0-100%)
- **IO19/IO20:** I2C SDA/SCL (GT911 Touch)

---

## 2. MODBUS RTU KOMUNIKACIJA
- **Protokol:** Modbus RTU Slave, 9600 8N1.
- **Adresa:** Podesiva (1-247), čuva se u NVS memoriji (default: 1).
- **Broadcast (ID 0):** Podrška za sinhronizaciju vremena i prognoze (bez odgovora).
- **Ključni Registri:**
    - `40001`: Target Temp (x10) - sinhronizovano sa `ui_ArcTemp` (10-40°C).
    - `40002`: HVAC Mode (0=OFF, 1=HEAT, 2=COOL).
    - `40022`: Relay Mode (0=3-Brzine, 1=1-Relej).
    - `40030-40046`: Weather Data (Prognoza za 5 dana).

---

## 3. LOGIKA SISTEMA I SIGURNOST
1.  **Relejni Deadband (100ms):** Obavezan delay pri prebacivanju brzina (Svi OFF -> 100ms -> Ciljani ON).
2.  **Window Sensor Priority:** Ako je prozor otvoren, HVAC ide u OFF. GUI prikazuje modalno upozorenje (ili Tile 2 onemogućen).
3.  **Weather Watchdog:** Ako registar `40030` nije ažuriran >12h, Tile 3 (Weather) se sakriva iz `ui_SwipeContainer`.
4.  **Inactivity Timeout:** Ako je sistem na bilo kojem `Settings` ekranu, nakon 30s bez touch-a, vraća se na `ui_Main`. Izuzetak je aktiviran WiFi Manager.
5.  **Clean Mode:** Blokira sav touch input na 60s (koristi `lv_layer_sys()`).

---

## 4. KORISNIČKI INTERFEJS (GUI)
Glavna navigacija koristi **ui_SwipeContainer** (lv_tileview) sa horizontalnim panelima:

### 4.1. Tile 1: Main (Home)
- Prikaz sata, unutrašnje/vanjske temperature.
- Dugme `ui_ButtonGoToThermostat` vodi direktno na Tile 2.
- Skriveni meni: Long press (5s) na `ui_ButtonHiddenMenu` (gornji lijevi ugao) otvara `ui_PinEntry`.

### 4.2. Tile 2: Thermostat
- **Setpoint:** Kontrola preko `ui_ArcTemp` i tastera `ui_BtnTempInc`/`ui_BtnTempDec` (Opseg: 10-40°C).
- **Fan Speed:** Ciklična promjena (Auto -> Low -> Mid -> High) preko `ui_BtnFan`.

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
