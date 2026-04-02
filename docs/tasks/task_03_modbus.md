# TASK 03: Modbus RTU Slave i Weather Register Mapping
**Cilj:** Implementirati RS485 komunikaciju (THVD1500) sa podrškom za vremensku prognozu i Broadcast ID 0.

**Upute:**
1.  **UART i RS485 Konfiguracija:**
    - Koristiti UART0 (Serial), Baud 115200, 8N1 (dijeli sa debug Serial).
    - Driver Enable (DE) pin: **IO41** (dedicated pin, bez sharinga).
    - Inicijalizacija: `s_mb.begin(&Serial, PIN_RS485_DE)`.
    - THVD1500: DE direktno na IO41, RE na GND (always receive capable).
2.  **Modbus Slave Adresa (NVS):**
    - Pročitati adresu (1-247) iz NVS. Ako ne postoji, koristiti default **1**.
3.  **Mapiranje Registara (FSD v4.0):**
    - **Holding Registri (40001-40046):**
        - `40001`: Target Temp (x10). Npr. 225 = 22.5°C. Sinhronizovano sa `ui_ArcTemp`.
        - `40002`: HVAC Mode (0=OFF, 1=HEAT, 2=COOL).
        - `40003`: Fan Speed (0=AUTO, 1=LOW, 2=MID, 3=HIGH).
        - `40022`: Relay Mode (0=3-Speed, 1=1-Relay).
        - **Weather Range (40030-40046):**
            - `40030`: Watchdog Timestamp (ili status).
            - `40031-40046`: Podaci za 5 dana (Dan, Ikona, Temp).
4.  **Broadcast ID 0 Logika:**
    - Prihvatiti upise na ID 0 bez slanja odgovora (ACK).
    - Iskoristiti ovo za masovno ažuriranje vremenske prognoze.
5.  **Weather Watchdog:**
    - Pri svakom upisu u `40030`, snimiti `millis()`.
    - Ako prođe >12h bez update-a, postaviti `Weather_Stale = true` (ovo očitava Task 06).
6.  **Input Registri — Ažuriranje iz HVAC Stanja:**
    - Relay status se čita preko `hal_relay_is_on(1/2/3)` — NE direktno sa GPIO pina.
    - Window sensor se čita preko `hal_window_is_open()` — NE direktno sa GPIO pina.
    - Ove vrijednosti se upisuju u Input registre za Modbus master.

**Očekivani rezultat:** Uređaj stabilno komunicira na RS485 sabirnici, IO41 ispravno kontroliše THVD1500 DE pin, i ispravno ažurira registre vremenske prognoze.
