// Custom UI event implementations — safe from SquareLine Studio overwrites
// Original content from lvgl/ui_events.c, moved here for persistence

#include <lvgl.h>
#include <Arduino.h>
#include <esp_heap_caps.h>

#include "hal.h"
#include "hvac.h"
#include "modbus_handler.h"
#include "settings.h"
#include "wifi_manager.h"
#include "debug_logger_c.h"

// Forward declarations for SquareLine-generated objects
extern lv_obj_t *ui_SwipeContainer;
extern lv_obj_t *ui_LabelCleanCountdown;
extern lv_obj_t *ui_LabelCleanText;
extern lv_obj_t *ui_LabelDay1, *ui_LabelDay2, *ui_LabelDay3, *ui_LabelDay4, *ui_LabelDay5;
extern lv_obj_t *ui_ImgIcon1, *ui_ImgIcon2, *ui_ImgIcon3, *ui_ImgIcon4, *ui_ImgIcon5;
extern lv_obj_t *ui_LabelTempHigh1, *ui_LabelTempHigh2, *ui_LabelTempHigh3, *ui_LabelTempHigh4, *ui_LabelTempHigh5;
extern lv_obj_t *ui_LabelTempLow1, *ui_LabelTempLow2, *ui_LabelTempLow3, *ui_LabelTempLow4, *ui_LabelTempLow5;
extern lv_obj_t *ui_TileWeather;
extern lv_obj_t *ui_TileThermostat;
extern lv_obj_t *ui_ArcTemp;
extern lv_obj_t *ui_LabelTargetTemp;
extern lv_obj_t *ui_LabelFanStatus;
extern lv_obj_t *ui_LabelCurrentTemp;
extern lv_obj_t *ui_LabelRoomTemp;
extern lv_obj_t *ui_LabelWeatherTodayDesc;
extern lv_obj_t *ui_ImageHeatStatus;
extern lv_obj_t *ui_ImageCoolStatus;
extern lv_obj_t *ui_PinEntry;
extern lv_obj_t *ui_PinTextArea;
extern lv_obj_t *ui_Settings1;
extern lv_obj_t *ui_Main;
extern lv_obj_t *ui_DropMinTemp;
extern lv_obj_t *ui_DropMaxTemp;
extern lv_obj_t *ui_DropMode;
extern lv_obj_t *ui_DropCtrlType;
extern lv_obj_t *ui_DropHysteresis;
extern lv_obj_t *ui_DropStageStep;
extern lv_obj_t *ui_SpinSensorOffset;
extern lv_obj_t *ui_SliderBrightHigh;
extern lv_obj_t *ui_SliderBrightLow;
extern lv_obj_t *ui_DropTimeout;
extern lv_obj_t *ui_SpinModbusAddr;
extern lv_obj_t *ui_SwitchStartAp;
extern lv_obj_t *ui_ButtonDnd;
extern lv_obj_t *ui_ButtonMur;

// Forward declarations for SquareLine-generated screens & helpers
extern void ui_PinEntry_screen_init(void);
extern void ui_Settings1_screen_init(void);
extern void ui_Settings2_screen_init(void);
extern void ui_Settings3_screen_init(void);
extern void ui_Main_screen_init(void);
extern void _ui_screen_change(lv_obj_t ** target, lv_scr_load_anim_t fademode, int spd, int delay, void (*target_init)(void));

// Settings3 init wrapper — initialise sliders from NVS values
static void ui_Settings3_screen_init_wrapped(void)
{
    ui_Settings3_screen_init();
    // Map NVS values (0-1023) → slider range (0-100)
    lv_slider_set_value(ui_SliderBrightHigh, g_sys_cfg.bright_high * 100 / 1023, LV_ANIM_OFF);
    lv_slider_set_value(ui_SliderBrightLow,  g_sys_cfg.bright_low  * 100 / 1023, LV_ANIM_OFF);
}

// Called every time Settings3 screen becomes visible (swipe or direct load)
void settings3_loaded_cb(lv_event_t *e)
{
    (void)e;
    lv_slider_set_value(ui_SliderBrightHigh, g_sys_cfg.bright_high * 100 / 1023, LV_ANIM_OFF);
    lv_slider_set_value(ui_SliderBrightLow,  g_sys_cfg.bright_low  * 100 / 1023, LV_ANIM_OFF);
}

// Forward declarations for PNG images (weather forecast icons)
extern const lv_img_dsc_t ui_img_sunny_png;      // WX_ICON_SUNNY (0)
extern const lv_img_dsc_t ui_img_sunny_day_png;  // WX_ICON_PARTLY_CLR (1)
extern const lv_img_dsc_t ui_img_hvac_png;       // Generic fallback icon

// ── Clean screen timer ────────────────────────────────────────────────────────
static lv_timer_t *s_clean_timer = NULL;
static uint8_t     s_clean_secs  = 60;

static void clean_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_clean_secs > 0) {
        s_clean_secs--;
        lv_label_set_text_fmt(ui_LabelCleanCountdown, "%u", s_clean_secs);
    } else {
        lv_timer_del(s_clean_timer);
        s_clean_timer = NULL;
        lv_obj_clear_flag(lv_layer_sys(), LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(ui_LabelCleanText, "Tap screen to start cleaning");
        lv_label_set_text(ui_LabelCleanCountdown, "");
    }
}

// ── Weather forecast ──────────────────────────────────────────────────────────
static const lv_img_dsc_t *weather_icon_src(uint8_t icon_id)
{
    // Map weather icon IDs to LVGL image assets
    // NOTE: Currently limited by available icons in SquareLine project
    // TODO: Add proper weather icons (cloudy, rainy, snowy, stormy, etc.)
    switch (icon_id) {
        case WX_ICON_SUNNY:       // 0 - Clear sky
            return &ui_img_sunny_png;
        case WX_ICON_PARTLY_CLR:  // 1 - Partly cloudy / Sunny day
            return &ui_img_sunny_day_png;
        case WX_ICON_CLOUDY:      // 2 - Cloudy (fallback to HVAC icon until proper icon added)
        case WX_ICON_RAINY:       // 3 - Rain (fallback)
        case WX_ICON_SNOWY:       // 4 - Snow (fallback)
        case WX_ICON_STORMY:      // 5 - Storm (fallback)
        case WX_ICON_FOGGY:       // 6 - Fog (fallback)
        case WX_ICON_WINDY:       // 7 - Wind (fallback)
        default:
            return &ui_img_hvac_png;  // Generic fallback icon
    }
}

static void update_forecast_card(uint8_t day_index, const forecast_data_t *data)
{
    lv_obj_t *lbl_day[]  = { ui_LabelDay1,      ui_LabelDay2,      ui_LabelDay3,
                              ui_LabelDay4,      ui_LabelDay5 };
    lv_obj_t *img_icon[] = { ui_ImgIcon1,        ui_ImgIcon2,        ui_ImgIcon3,
                              ui_ImgIcon4,        ui_ImgIcon5 };
    lv_obj_t *lbl_high[] = { ui_LabelTempHigh1,  ui_LabelTempHigh2,  ui_LabelTempHigh3,
                              ui_LabelTempHigh4,  ui_LabelTempHigh5 };
    lv_obj_t *lbl_low[]  = { ui_LabelTempLow1,   ui_LabelTempLow2,   ui_LabelTempLow3,
                              ui_LabelTempLow4,   ui_LabelTempLow5 };

    if (day_index >= WX_MAX_DAYS) return;

    lv_label_set_text(lbl_day[day_index], data->day_name);
    lv_img_set_src(img_icon[day_index], weather_icon_src(data->icon_id));

    char buf[8];
    snprintf(buf, sizeof(buf), "%d\xC2\xB0", data->temp_high_c10 / 10);
    lv_label_set_text(lbl_high[day_index], buf);
    lv_obj_set_style_text_color(lbl_high[day_index], lv_color_hex(0xFF0000), LV_PART_MAIN);

    snprintf(buf, sizeof(buf), "%d\xC2\xB0", data->temp_low_c10 / 10);
    lv_label_set_text(lbl_low[day_index], buf);
    lv_obj_set_style_text_color(lbl_low[day_index], lv_color_hex(0x0000FF), LV_PART_MAIN);
}

void update_weather_forecast_ui(void)
{
    if (g_mb.weather_stale) {
        lv_obj_add_flag(ui_TileWeather, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(ui_TileWeather, LV_OBJ_FLAG_HIDDEN);

    for (uint8_t i = 0; i < WX_MAX_DAYS; i++) {
        if (g_mb.wx_days[i].day_name[0] == '\0') continue;
        update_forecast_card(i, &g_mb.wx_days[i]);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Navigation & Main Tile
// ═════════════════════════════════════════════════════════════════════════════

void action_go_to_thermo(lv_event_t *e)
{
    (void)e;
    lv_obj_scroll_to_view(ui_TileThermostat, LV_ANIM_ON);
}

void action_dnd_toggled(lv_event_t * e)
{
    (void)e;
    inactivity_reset();
    bool checked = lv_obj_has_state(ui_ButtonDnd, LV_STATE_CHECKED);
    LOG_C_INFO("[UI] DND toggled %s", checked ? "ON" : "OFF");

    if (checked) {
        // DND is activated, so deactivate MUR
        lv_obj_clear_state(ui_ButtonMur, LV_STATE_CHECKED);
        modbus_set_mur_coil(false);
        LOG_C_INFO("[UI] MUR state cleared due to DND activation");
    }
    modbus_set_dnd_coil(checked);
}

void action_mur_toggled(lv_event_t * e)
{
    (void)e;
    inactivity_reset();
    bool checked = lv_obj_has_state(ui_ButtonMur, LV_STATE_CHECKED);
    LOG_C_INFO("[UI] MUR toggled %s", checked ? "ON" : "OFF");

    if (checked) {
        // MUR is activated, so deactivate DND
        lv_obj_clear_state(ui_ButtonDnd, LV_STATE_CHECKED);
        modbus_set_dnd_coil(false);
        LOG_C_INFO("[UI] DND state cleared due to MUR activation");
    }
    modbus_set_mur_coil(checked);
}

void action_hidden_menu_press(lv_event_t *e)
{
    (void)e;
    _ui_screen_change(&ui_PinEntry, LV_SCR_LOAD_ANIM_FADE_ON,
                      500, 0, &ui_PinEntry_screen_init);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Thermostat Tile
// ═════════════════════════════════════════════════════════════════════════════

void action_temp_inc(lv_event_t *e)
{
    (void)e;
    int val = lv_arc_get_value(ui_ArcTemp);
    if (val < TEMP_SETPOINT_MAX) {
        val++;
        lv_arc_set_value(ui_ArcTemp, val);
        lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", val);
        hvac_set_setpoint(val);
    }
}

void action_temp_dec(lv_event_t *e)
{
    (void)e;
    int val = lv_arc_get_value(ui_ArcTemp);
    if (val > TEMP_SETPOINT_MIN) {
        val--;
        lv_arc_set_value(ui_ArcTemp, val);
        lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", val);
        hvac_set_setpoint(val);
    }
}

void action_fan_speed_change(lv_event_t *e)
{
    (void)e;
    
    // Pročitaj trenutnu vrednost iz Modbus registra
    uint8_t current_speed = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];
    
    LOG_C_INFO("[UI] Fan button clicked, current speed from Modbus: %u", current_speed);
    
    // Inkrementiraj cirkularno (0->1->2->3->0)
    current_speed = (current_speed + 1) % 4;
    
    LOG_C_INFO("[UI] New fan speed: %u", current_speed);
    
    // Ažuriraj tekst na labeli prema novoj vrednosti
    switch (current_speed) {
        case FAN_AUTO: lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
        case FAN_LOW:  lv_label_set_text(ui_LabelFanStatus, "Low");  break;
        case FAN_MID:  lv_label_set_text(ui_LabelFanStatus, "Mid");  break;
        case FAN_HIGH: lv_label_set_text(ui_LabelFanStatus, "High"); break;
    }
    
    // Postavi novu vrednost u HVAC i Modbus
    hvac_set_fan_speed(current_speed);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Clean Screen (Tile 4)
// ═════════════════════════════════════════════════════════════════════════════

void action_clean_start(lv_event_t *e)
{
    (void)e;
    if (s_clean_timer) return;

    s_clean_secs = 60;
    lv_obj_add_flag(lv_layer_sys(), LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(ui_LabelCleanText, "CLEANING...");
    lv_label_set_text_fmt(ui_LabelCleanCountdown, "%u", s_clean_secs);
    s_clean_timer = lv_timer_create(clean_timer_cb, 1000, NULL);
}

// ═════════════════════════════════════════════════════════════════════════════
//  PIN Entry
// ═════════════════════════════════════════════════════════════════════════════

void action_validate_pin(lv_event_t *e)
{
    (void)e;
    const char *pin = lv_textarea_get_text(ui_PinTextArea);
    if (strcmp(pin, "43962") == 0) {
        inactivity_set_on_settings(true);
        _ui_screen_change(&ui_Settings1, LV_SCR_LOAD_ANIM_FADE_ON,
                          500, 0, &ui_Settings1_screen_init);
    } else {
        _ui_screen_change(&ui_Main, LV_SCR_LOAD_ANIM_FADE_ON,
                          500, 0, &ui_Main_screen_init);
    }
    lv_textarea_set_text(ui_PinTextArea, "");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Settings 1 — Basic
// ═════════════════════════════════════════════════════════════════════════════

void action_min_temp_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropMinTemp);
    g_sys_cfg.temp_min = (int16_t)(10 + sel);
    g_dirty_flags |= FLAG_TEMP_MIN;
}

void action_max_temp_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropMaxTemp);
    g_sys_cfg.temp_max = (int16_t)(20 + sel);
    g_dirty_flags |= FLAG_TEMP_MAX;
}

void action_hvac_mode_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropMode);
    g_sys_cfg.hvac_mode = (uint8_t)sel;
    g_dirty_flags |= FLAG_HVAC_MODE;
    hvac_set_mode(g_sys_cfg.hvac_mode);
}

void action_ctrl_type_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropCtrlType);
    g_sys_cfg.ctrl_type = (uint8_t)sel;
    g_dirty_flags |= FLAG_CTRL_TYPE;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Settings 2 — Advanced
// ═════════════════════════════════════════════════════════════════════════════

void action_hysteresis_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropHysteresis);
    g_sys_cfg.hysteresis_x10 = (int16_t)((sel + 1) * 5);
    g_dirty_flags |= FLAG_HYSTERESIS;
}

void action_stage_step_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    uint16_t sel = lv_dropdown_get_selected(ui_DropStageStep);
    g_sys_cfg.stage_step_x10 = (int16_t)((sel + 1) * 5);
    g_dirty_flags |= FLAG_STAGE_STEP;
}

void action_offset_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    int32_t val = lv_spinbox_get_value(ui_SpinSensorOffset);
    g_sys_cfg.sensor_offset_x10 = (int16_t)val;
    g_dirty_flags |= FLAG_SENSOR_OFFSET;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Settings 3 — System
// ═════════════════════════════════════════════════════════════════════════════

void action_bright_high_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    int32_t val = lv_slider_get_value(ui_SliderBrightHigh);
    uint16_t mapped = (uint16_t)(val * 1023 / 100);
    g_sys_cfg.bright_high = mapped;
    g_dirty_flags |= FLAG_BRIGHT_HIGH;
    hal_backlight_set(mapped);
}

void action_bright_low_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    int32_t val = lv_slider_get_value(ui_SliderBrightLow);
    g_sys_cfg.bright_low = (uint16_t)(val * 1023 / 100);
    g_dirty_flags |= FLAG_BRIGHT_LOW;
}

void action_timeout_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    static const uint8_t timeout_table[] = {15, 30, 60, 120};
    uint16_t sel = lv_dropdown_get_selected(ui_DropTimeout);
    if (sel < sizeof(timeout_table))
        g_sys_cfg.timeout_s = timeout_table[sel];
    g_dirty_flags |= FLAG_TIMEOUT;
}

void action_modbus_changed(lv_event_t *e)
{
    (void)e;
    inactivity_reset();
    int32_t val = lv_spinbox_get_value(ui_SpinModbusAddr);
    if (val < 1)   val = 1;
    if (val > 247) val = 247;
    g_sys_cfg.modbus_addr = (uint8_t)val;
    g_dirty_flags |= FLAG_MODBUS_ADDR;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Save & Exit
// ═════════════════════════════════════════════════════════════════════════════

void action_save_and_exit(lv_event_t *e)
{
    (void)e;
    settings_save_dirty();
    inactivity_set_on_settings(false);
    _ui_screen_change(&ui_Main, LV_SCR_LOAD_ANIM_FADE_ON,
                      500, 0, &ui_Main_screen_init);
}

// ═════════════════════════════════════════════════════════════════════════════
//  WiFi AP Manager
// ═════════════════════════════════════════════════════════════════════════════

void action_start_wifi_manager(lv_event_t *e)
{
    (void)e;
    bool ap_on = lv_obj_has_state(ui_SwitchStartAp, LV_STATE_CHECKED);
    g_wifi_ap_active = ap_on;
    inactivity_reset();
    wifi_manager_set_ap(ap_on);
}
