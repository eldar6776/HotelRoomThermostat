# TASK 06: Weather Watchdog i Weather UI (Tile 3)
**Cilj:** Implementirati ažuriranje 5 statičkih kartica vremenske prognoze na osnovu Modbus podataka.

**Upute:**
1.  **Modbus Weather Data (Registers 40030-40046):**
    - Mapirati podatke za 5 dana: Ime dana, Ikonu (Cooling/Heating/Sunny), Maksimalnu (High) i Minimalnu (Low) temperaturu.
2.  **Ažuriranje Statičkih Objekata (ui_Main.c):**
    - Interfejs koristi 5 pre-kreiranih kontejnera (`CardDay1` do `CardDay5`).
    - Implementirati funkciju `update_forecast_card(uint8_t day_index, forecast_data_t * data)` koja:
        - Za svaki `day_index` (1-5) ažurira labele: `ui_LabelDayX`, `ui_LabelTempHighX`, `ui_LabelTempLowX`.
        - Postavlja izvor slike za `ui_ImgIconX`.
3.  **Vizuelni Standardi:**
    - Maksimalna temperatura (`High`) mora biti crvene boje (`#FF0000`).
    - Minimalna temperatura (`Low`) mora biti plave boje (`#0000FF`).
4.  **Weather Watchdog (12h limit):**
    - Pratiti zadnje vrijeme upisa u `40030`.
    - Ako su podaci "Stale" (>12h), sakriti cijeli Tile 3 iz `ui_SwipeContainer`.
    - Čim stigne novi upis, Tile 3 mora postati ponovo vidljiv i ažuriran.

**Očekivani rezultat:** Pregledna vremenska prognoza na statičkim karticama sa jasno istaknutim dnevnim i noćnim temperaturama.
