# TASK 07: Sigurnost, Settings i NVS Wear Leveling
**Cilj:** Implementirati PIN zaštitu, kružne postavke sa Dirty Flag logikom i NVS memorijom.

**Upute:**
1.  **PIN Zaštita (ui_PinEntry.c):**
    - `action_validate_pin`: Ako je unesen PIN "1234", preći na `ui_Settings1`. U suprotnom, vratiti na `ui_Main`.
2.  **Dirty Flag Bitmask (Settings 1-3):**
    - Definisati `uint32_t dirty_flags = 0`.
    - Svaka `action_..._changed` funkcija na Settings ekranima (npr. `action_min_temp_changed`) mora:
        1. Očitati vrijednost sa LVGL objekta (Dropdown, Slider, Spinbox).
        2. Ažurirati privremenu RAM strukturu `sys_config`.
        3. Podići odgovarajući bit u `dirty_flags` (npr. `dirty_flags |= FLAG_TEMP_MIN`).
3.  **Save & Exit (action_save_and_exit):**
    - Funkciju poziva bilo koji `BtnSave` na Settings ekranima.
    - Ako je `dirty_flags != 0`, pozvati funkciju za upis izmijenjenih polja u NVS (ESP32 Preferences).
    - Resetovati `dirty_flags` na 0 i vratiti se na `ui_Main`.
4.  **Inactivity Timeout:**
    - Ako je sistem na bilo kojem Settings ekranu (`Settings1`, `Settings2`, `Settings3`), pokrenuti tajmer.
    - Ako nema touch-a 30 sekundi, automatski se vratiti na `ui_Main` bez snimanja promjena.
    - Resetovati tajmer na svaki `LV_EVENT_PRESSED`.
5.  **WiFi Manager (Maintenance):**
    - `action_start_wifi_manager` (`ui_SwitchStartAp`): Ako je Switch uključen, pauzirati Inactivity Timeout i pokrenuti ESP32 u AP modu.

**Očekivani rezultat:** Siguran pristup postavkama, efikasan upis u NVS samo pri promjenama i automatski povratak na glavni ekran u slučaju neaktivnosti.
