# TASK 05: LVGL Navigacija i UI Callbacks (TileView)
**Cilj:** Implementirati funkcionalnost `ui_SwipeContainer` i povezati sve callbacke sa glavnog ekrana.

**Upute:**
1.  **Main Screen Callbacks (ui_Main.c):**
    - `action_go_to_thermo`: Pozvati `lv_obj_set_tile_id(ui_SwipeContainer, 1, 0, LV_ANIM_ON)`.
    - `action_hidden_menu_press`: Detektovati Long Press (5s) na `ui_ButtonHiddenMenu` i izvršiti `_ui_screen_change(&ui_PinEntry, ...)`.
2.  **Thermostat Callbacks (ui_Main.c):**
    - `action_temp_inc` i `action_temp_dec`: Ažurirati `ui_ArcTemp` (opseg 10-40) i sinhronizovati `ui_LabelTargetTemp`.
    - `action_fan_speed_change`: Kružno mijenjati statičku varijablu `fan_state` (0=Auto, 1=Low, 2=Mid, 3=High) i ažurirati `ui_LabelFanStatus`.
3.  **DND / MUR Logika (Exclusivity):**
    - Podesiti dugmad `ui_BtnDND` i `ui_BtnMUR` kao prekidače.
    - Ako se aktivira DND -> Isključi MUR (i obrnuto). Stanje slati u Modbus Coile `00001` i `00002`.
4.  **Weather Watchdog (Tile 3):**
    - Provjeriti timestamp zadnjeg Modbus upisa u registar `40030`.
    - Ako je prošlo >12h, sakriti Tile 3 (`ui_TileWeather`) iz `ui_SwipeContainer` kako swipe ne bi bio moguć.
5.  **Clean Screen (Tile 4):**
    - `action_clean_start`: Blokirati sav touch input na 60s koristeći `lv_layer_sys()`.
    - Prikazati odbrojavanje u `ui_LabelCleanText`.

**Očekivani rezultat:** Potpuna interaktivnost na Main ekranu, funkcionalan Thermostat tile i siguran Clean mode.
