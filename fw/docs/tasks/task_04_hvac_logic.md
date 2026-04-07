# TASK 04: HVAC Logika, Deadband i Setpoint Sinhronizacija
**Cilj:** Implementirati termostatsku logiku, upravljanje relejima preko I2C expandera i očitavanje temperature.

**Upute:**
1.  **NTC Termistor (IO1) i ADC:**
    - Očitavati `IO1` (ADC1_CH0). Koristiti Beta formulu (B=3950, R=100k) za °C.
    - Implementirati Exponential Moving Average (EMA) filter za stabilnost.
    - Vrijednost slati u `ui_LabelCurrentTemp` (Main) i `ui_LabelCurrTempThermostat` (Thermo).
2.  **Setpoint i Fan Speed (UI -> HVAC):**
    - Setpoint (Target Temp) je definisan preko `ui_ArcTemp` (Opseg 10-40°C).
    - Fan Speed ciklus (Auto, Low, Mid, High) se kontroliše u `action_fan_speed_change`.
    - Rezultati moraju biti upisani u Modbus registre `40001` (Setpoint) i `40003` (Fan Speed).
3.  **100ms Deadband i Relejni Interlock:**
    - Pri svakoj promjeni brzine u `3-Speed` modu (40022=0):
        1. Pozvati `hal_relay_all_off()` — gasi sve releje preko I2C expandera.
        2. Čekaj 100ms (neblokirajući `millis()` ili `vTaskDelay`).
        3. Pozvati `hal_relay_set(target_id, true)` — uključuje samo ciljani relej.
    - ⚠️ VAŽNO: NE koristiti `digitalWrite()` za releje — svi releji su na I2C expanderu!
4.  **Adapter API za Releje (`hal.h`):**
    - `hal_relay_set(id, on)` — postavlja stanje releja (id: 1,2,3).
    - `hal_relay_is_on(id)` — provjerava da li je relej aktivan (za hysteresis logiku).
    - `hal_relay_all_off()` — gasi sve releje odjednom (za deadband i mode change).
    - Ove funkcije automatski rade za PCF8574AN (inverzna logika) i PCA9554ADH (direktna logika).
5.  **Sigurnost (Window Sensor):**
    - Window Sensor je na I2C expander P6 (Active LOW).
    - Čitati stanje preko `hal_window_is_open()` — NE direktno sa GPIO pina.
    - Ako je prozor otvoren (`hal_window_is_open() == true`):
        - Pozvati `hal_relay_all_off()` — gasi sve releje.
        - Forsiraj HVAC u OFF mod.
        - Onemogući kontrolu preko GUI-ja dok se prozor ne zatvori.
        - Prikazati "WINDOW OPEN" status na ekranu.

**Očekivani rezultat:** Stabilno očitavanje temperature, precizna kontrola releja sa deadband-om preko I2C expandera, sinhronizacija sa UI elementima, i ispravna detekcija otvorenog prozora.
