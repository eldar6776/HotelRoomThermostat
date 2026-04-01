# UI Event Logic & Callback Definitions
**Project:** HotelRoomThermostat  
**LVGL Version:** 8.3.11  
**Resolution:** 480x480  
**Hardware Target:** ESP32 (Embedded)

---

## 1. Main Screen Events (`ui_Main.c`)

### `action_go_to_thermo`
* **Trigger:** `LV_EVENT_CLICKED` na `ui_ButtonGoToThermostat`.
* **Opis:** Vrši navigaciju na Thermostat tile unutar `ui_SwipeContainer`.
* **Logika:** Poziva `lv_obj_set_tile_id` za prebacivanje ekrana.
~~~c
void action_go_to_thermo(lv_event_t * e) {
    // Indeksiranje se mora uskladiti sa redoslijedom dodavanja u ui_SwipeContainer
    lv_obj_set_tile_id(ui_SwipeContainer, 1, 0, LV_ANIM_ON);
}
~~~

### `action_hidden_menu_press`
* **Trigger:** `LV_EVENT_LONG_PRESSED` na `ui_ButtonHiddenMenu`.
* **Opis:** Aktivira skriveni ulaz za servisni meni.
* **Logika:** Vrši promjenu ekrana na `ui_PinEntry`.
~~~c
void action_hidden_menu_press(lv_event_t * e) {
    _ui_screen_change(&ui_PinEntry, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_PinEntry_screen_init);
}
~~~

---

## 2. Thermostat Controls (`ui_Main.c`)

### `action_temp_inc`
* **Trigger:** `LV_EVENT_CLICKED` i `LV_EVENT_LONG_PRESSED_REPEAT` na `ui_BtnTempInc`.
* **Opis:** Povećava zadanu temperaturu (Setpoint).
* **Logika:** Očitava trenutnu vrijednost sa `ui_ArcTemp`, inkrementira je i ažurira Arc i Label objekat.
~~~c
void action_temp_inc(lv_event_t * e) {
    int current_temp = lv_arc_get_value(ui_ArcTemp);
    if(current_temp < 40) { 
        current_temp++;
        lv_arc_set_value(ui_ArcTemp, current_temp);
        lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", current_temp);
    }
}
~~~

### `action_temp_dec`
* **Trigger:** `LV_EVENT_CLICKED` i `LV_EVENT_LONG_PRESSED_REPEAT` na `ui_BtnTempDec`.
* **Opis:** Smanjuje zadanu temperaturu.
* **Logika:** Dekrementira vrijednost uz poštovanje donje granice.
~~~c
void action_temp_dec(lv_event_t * e) {
    int current_temp = lv_arc_get_value(ui_ArcTemp);
    if(current_temp > 10) { 
        current_temp--;
        lv_arc_set_value(ui_ArcTemp, current_temp);
        lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", current_temp);
    }
}
~~~

### `action_fan_speed_change`
* **Trigger:** `LV_EVENT_CLICKED` na `ui_BtnFan`.
* **Opis:** Kružna promjena brzine ventilatora (Auto -> Low -> Mid -> High).
* **Logika:** Održava statičku varijablu stanja i mijenja tekst na `ui_LabelFanStatus`.
~~~c
void action_fan_speed_change(lv_event_t * e) {
    static uint8_t fan_state = 0;
    // 0: Auto, 1: Low, 2: Mid, 3: High
    fan_state = (fan_state + 1) % 4;
    switch(fan_state) {
        case 0: lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
        case 1: lv_label_set_text(ui_LabelFanStatus, "Low"); break;
        case 2: lv_label_set_text(ui_LabelFanStatus, "Mid"); break;
        case 3: lv_label_set_text(ui_LabelFanStatus, "High"); break;
    }
}
~~~

---

## 3. Security & Access (`ui_PinEntry.c`)

### `action_validate_pin`
* **Trigger:** `LV_EVENT_READY` (OK/Checkmark taster) na `ui_Keyboard1`.
* **Opis:** Provjerava PIN unesen u `ui_PinTextArea`.
* **Logika:** Upoređuje string. Ako je ispravan prelazi na `ui_Settings1`, ako nije vraća na `ui_Main`.
~~~c
void action_validate_pin(lv_event_t * e) {
    const char * entered_pin = lv_textarea_get_text(ui_PinTextArea);
    if(strcmp(entered_pin, "1234") == 0) { 
        _ui_screen_change(&ui_Settings1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Settings1_screen_init);
    } else {
        _ui_screen_change(&ui_Main, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Main_screen_init);
    }
    lv_textarea_set_text(ui_PinTextArea, "");
}
~~~

---

## 4. Maintenance & Service (`ui_Main.c`)

### `action_clean_start`
* **Trigger:** `LV_EVENT_CLICKED` na `ui_BtnCleanStart`.
* **Opis:** Pokreće proceduru čišćenja ekrana.
* **Logika:** Apsolutna blokada svih sistemskih touch unosa.
~~~c
void action_clean_start(lv_event_t * e) {
    // Blokada touch inputa na najvišem sistemskom sloju
    lv_obj_add_flag(lv_layer_sys(), LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(ui_LabelCleanText, "CLEANING...");
}
~~~

---

## 5. Weather Forecast Update Logic (`ui_Main.c` / `weather_api.c`)

Interfejs koristi 5 statičkih kontejnera (`CardDay1` do `CardDay5`) kreiranih u SquareLine Studiju. Svaka kartica ima definisan obrub, senku i odvojene labele za dnevnu (High) i noćnu (Low) temperaturu.

**Estetika kartica u SquareLine Studiju:**
* **Boja maksimalne temperature:** Crvena (`#FF0000`).
* **Boja minimalne temperature:** Plava (`#0000FF`).
* **Obrub kartice:** Width `1` (ili `2`).
* **Senka kartice:** Shadow Width `5`, Shadow Offset Y `2`.

Coding agent mora implementirati sljedeću logiku za parsiranje JSON podataka i ažuriranje UI elemenata.

```c
// Struktura za prosljeđivanje parsiranih API podataka
typedef struct {
    const char * day_name;
    const char * temp_high_str;   // Npr. "15°"
    const char * temp_low_str;    // Npr. "8°"
    const void * icon_src;   
} forecast_data_t;

// Funkcija se poziva u petlji za svaki od 5 dana
void update_forecast_card(uint8_t day_index, forecast_data_t * data) {
    switch(day_index) {
        case 1:
            lv_label_set_text(ui_LabelDay1, data->day_name);
            lv_label_set_text(ui_LabelTempHigh1, data->temp_high_str);
            lv_label_set_text(ui_LabelTempLow1, data->temp_low_str);
            lv_img_set_src(ui_ImgIcon1, data->icon_src);
            break;
        case 2:
            lv_label_set_text(ui_LabelDay2, data->day_name);
            lv_label_set_text(ui_LabelTempHigh2, data->temp_high_str);
            lv_label_set_text(ui_LabelTempLow2, data->temp_low_str);
            lv_img_set_src(ui_ImgIcon2, data->icon_src);
            break;
        case 3:
            lv_label_set_text(ui_LabelDay3, data->day_name);
            lv_label_set_text(ui_LabelTempHigh3, data->temp_high_str);
            lv_label_set_text(ui_LabelTempLow3, data->temp_low_str);
            lv_img_set_src(ui_ImgIcon3, data->icon_src);
            break;
        case 4:
            lv_label_set_text(ui_LabelDay4, data->day_name);
            lv_label_set_text(ui_LabelTempHigh4, data->temp_high_str);
            lv_label_set_text(ui_LabelTempLow4, data->temp_low_str);
            lv_img_set_src(ui_ImgIcon4, data->icon_src);
            break;
        case 5:
            lv_label_set_text(ui_LabelDay5, data->day_name);
            lv_label_set_text(ui_LabelTempHigh5, data->temp_high_str);
            lv_label_set_text(ui_LabelTempLow5, data->temp_low_str);
            lv_img_set_src(ui_ImgIcon5, data->icon_src);
            break;
    }
}
~~~

---

## 6. Settings Architecture & Data Management

Podešavanja su podijeljena na tri ekrana bez ugniježđenih kontejnera, koristeći apsolutno pozicioniranje radi uštede RAM-a mikrokontrolera.

### NVS Wear Leveling (Dirty Flag Bitmask)
Sve promjene u UI elementima se zapisuju isključivo u RAM lokalne varijable (npr. strukturu `sys_config`). Praćenje promjena vrši se putem 32-bitne maske. Upis u flash (NVS) dešava se tek kada korisnik okine funkciju `action_save_and_exit`.

~~~c
typedef enum {
    FLAG_TEMP_MIN      = (1 << 0), 
    FLAG_TEMP_MAX      = (1 << 1), 
    FLAG_HVAC_MODE     = (1 << 2), 
    FLAG_CTRL_TYPE     = (1 << 3), 
    FLAG_HYSTERESIS    = (1 << 4), 
    FLAG_STAGE_STEP    = (1 << 5),
    FLAG_SENSOR_OFFSET = (1 << 6),
    FLAG_BRIGHT_HIGH   = (1 << 7),
    FLAG_BRIGHT_LOW    = (1 << 8),
    FLAG_TIMEOUT       = (1 << 9),
    FLAG_MODBUS_ADDR   = (1 << 10)
} settings_flag_t;

extern uint32_t dirty_flags;
~~~

### Inactivity Return Timeout Logika
Hardkodirani sigurnosni mehanizam za automatski povratak sa bilo kojeg Settings ekrana nazad na `ui_Main`.
* **Osnovno pravilo:** Timeout je fiksiran na **30 sekundi** i resetuje se pri svakom registrovanom `LV_EVENT_PRESSED` događaju na ekranu. Ako tajmer istekne, sistem vrši prelaz na `ui_Main` i briše RAM promjene.
* **Exception (WiFi Manager):** Ukoliko se aktivira WiFi AP mod (preko `ui_SwitchStartAp`), tajmer za Inactivity Timeout se **pauzira**. Ako korisnik isključi switch bez restarta sistema, tajmer nastavlja odbrojavanje od 30 sekundi od tog trenutka.

---

## 7. Očekivane C Funkcije (Za Implementaciju)

Coding agent mora implementirati sljedeće `action_...` callback funkcije definisane u GUI kodu. Svaka funkcija koja kontroliše vrijednosti treba:
1. Očitati novu vrijednost sa LVGL objekta.
2. Ažurirati globalnu RAM strukturu konfiguracije.
3. Podići (`|=`) odgovarajući bit u `dirty_flags` maski.

### `ui_Settings1` (Osnovne Postavke)
* `void action_min_temp_changed(lv_event_t * e)` -> Izvor: `ui_DropMinTemp`. Maska: `FLAG_TEMP_MIN`.
* `void action_max_temp_changed(lv_event_t * e)` -> Izvor: `ui_DropMaxTemp`. Maska: `FLAG_TEMP_MAX`.
* `void action_hvac_mode_changed(lv_event_t * e)` -> Izvor: `ui_DropMode`. Maska: `FLAG_HVAC_MODE`.
* `void action_ctrl_type_changed(lv_event_t * e)` -> Izvor: `ui_DropCtrlType`. Maska: `FLAG_CTRL_TYPE`.

### `ui_Settings2` (Napredne Postavke)
* `void action_hysteresis_changed(lv_event_t * e)` -> Izvor: `ui_DropHysteresis`. Maska: `FLAG_HYSTERESIS`.
* `void action_stage_step_changed(lv_event_t * e)` -> Izvor: `ui_DropStageStep`. Maska: `FLAG_STAGE_STEP`.
* `void action_offset_changed(lv_event_t * e)` -> Izvor: `ui_SpinSensorOffset`. Dohvatiti vrijednost preko `lv_spinbox_get_value`. Maska: `FLAG_SENSOR_OFFSET`.

### `ui_Settings3` (Sistem i Komunikacija)
* `void action_bright_high_changed(lv_event_t * e)` -> Izvor: `ui_SliderBrightHigh`. Maska: `FLAG_BRIGHT_HIGH`.
* `void action_bright_low_changed(lv_event_t * e)` -> Izvor: `ui_SliderBrightLow`. Maska: `FLAG_BRIGHT_LOW`.
* `void action_timeout_changed(lv_event_t * e)` -> Izvor: `ui_DropTimeout`. Maska: `FLAG_TIMEOUT`.
* `void action_modbus_changed(lv_event_t * e)` -> Izvor: `ui_SpinModbusAddr`. Maska: `FLAG_MODBUS_ADDR`.

### Globalne / Izvršne Funkcije
* `void action_start_wifi_manager(lv_event_t * e)`
    * **Izvor:** `ui_SwitchStartAp` (`LV_EVENT_VALUE_CHANGED`).
    * **Logika:** Provjeriti stanje switcha (`lv_obj_has_state(ui_SwitchStartAp, LV_STATE_CHECKED)`). Prekinuti/Pauzirati sistemski "Inactivity Timeout", te pokrenuti/ugasiti ESP32 AP mode za mrežnu konfiguraciju.
* `void action_save_and_exit(lv_event_t * e)`
    * **Izvor:** Pozvana preko `BtnSave1`, `BtnSave2` ili `BtnSave3` (`LV_EVENT_CLICKED`).
    * **Logika:** Mora provjeriti `dirty_flags`. Ako je maska `!= 0`, inicijalizovati NVS upis za bitove koji su postavljeni na 1. Resetovati `dirty_flags` na 0, zatim izvršiti navigaciju `_ui_screen_change(&ui_Main, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Main_screen_init);`.
* **Cirkularna NEXT Navigacija** (Implementirana unutar SquareLine Studija):
    * `BtnNext1` (`LV_EVENT_CLICKED`) poziva promjenu na `ui_Settings2`.
    * `BtnNext2` (`LV_EVENT_CLICKED`) poziva promjenu na `ui_Settings3`.
    * `BtnNext3` (`LV_EVENT_CLICKED`) poziva promjenu nazad na `ui_Settings1`.

---

## 8. Tehničke napomene za implementaciju
* **Zabrana pretpostavki:** Kodiranje se mora bazirati isključivo na dostavljenim `.c` i `.h` fajlovima interfejsa. Nikada ne pretpostavljati GPIO pinove, Modbus adrese ili registre ukoliko nisu eksplicitno navedeni u dokumentaciji hardvera.
* **Referenca objekata:** Svi UI objekti (npr. `ui_ArcTemp`, `ui_ContainerForecast`) moraju se tretirati kao `extern` i pristupati im se instanciranjem zaglavlja `ui.h`.
* **Animacije:** Za sve UI prelaze strogo koristiti `LV_SCR_LOAD_ANIM_FADE_ON` parametar sa trajanjem od `500ms` za dosljedno korisničko iskustvo.
* **Izbjegavanje analogija:** Dokumentacija i kodni komentari se baziraju isključivo na C sintaksi i LVGL v8 arhitekturi. Apsolutno je zabranjeno poređenje sa strukturama i modelima iz drugih programskih jezika ili grafičkih biblioteka.