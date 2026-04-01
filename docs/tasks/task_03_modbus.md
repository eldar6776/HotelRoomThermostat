# TASK 03: Modbus RTU Slave i Weather Register Mapping
**Cilj:** Implementirati RS485 komunikaciju (THVD1500) sa podrškom za vremensku prognozu i Broadcast ID 0.

**Upute:**
1.  **UART i RS485 Konfiguracija:**
    - Koristiti UART2, Baud 9600, 8N1.
    - Automatska RTS kontrola na **IO48** (`UART_MODE_RS485_HALF_DUPLEX`).
2.  **Modbus Slave Adresa (NVS):**
    - Pročitati adresu (1-247) iz NVS. Ako ne postoji, koristiti default **1**.
3.  **Mapiranje Registara (FSD v3.0):**
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

**Očekivani rezultat:** Uređaj stabilno komunicira na RS485 sabirnici i ispravno ažurira registre vremenske prognoze.
