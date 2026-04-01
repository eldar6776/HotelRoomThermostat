# TASK 04: HVAC Logika, Deadband i Setpoint Sinhronizacija
**Cilj:** Implementirati termostatsku logiku, upravljanje relejima i očitavanje temperature.

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
        1. Isključi SVE releje (IO40, IO2, IO39).
        2. Čekaj 100ms (neblokirajući `millis()` ili `vTaskDelay`).
        3. Uključi samo ciljani relej.
4.  **Sigurnost (Window Sensor):**
    - Ako je `IO41` LOW (Prozor otvoren):
        - Forsiraj HVAC u OFF mod.
        - Onemogući kontrolu preko GUI-ja dok se prozor ne zatvori.
        - Prikazati "WINDOW OPEN" status na ekranu.

**Očekivani rezultat:** Stabilno očitavanje temperature, precizna kontrola releja sa deadband-om i sinhronizacija sa UI elementima.
