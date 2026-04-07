#include "settings.h"
#include <Preferences.h>
#include "debug_logger.h"
#include <lvgl.h>

// Forward declare ui_Main for screen-change use in inactivity callback
#ifdef __cplusplus
extern "C" {
#endif
#include "../lvgl/ui.h"
bool ui_clean_countdown_active(void);
#ifdef __cplusplus
}
#endif

// ── Globals ───────────────────────────────────────────────────────────────────
sys_config_t g_sys_cfg;
uint32_t     g_dirty_flags  = 0;
bool         g_wifi_ap_active = false;

static Preferences s_prefs;
static bool        s_on_settings = false;
static unsigned long s_last_touch_ms = 0;

static void inactivity_apply_screensaver(void)
{
    // Dim display on timeout, then return to Main from any screen.
    hal_backlight_set(g_sys_cfg.bright_low);

    lv_obj_t *active = lv_scr_act();
    bool on_settings_screen =
        (active == ui_Settings1) ||
        (active == ui_Settings2) ||
        (active == ui_Settings3);

    if (on_settings_screen || s_on_settings) {
        settings_reset_dirty();  // discard unsaved settings edits
        s_on_settings = false;
    }

    if (active != ui_Main) {
        _ui_screen_change(&ui_Main, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Main_screen_init);
    }
}

// ── NVS namespace ─────────────────────────────────────────────────────────────
#define NVS_NS "thermostat"

// ── settings_init ─────────────────────────────────────────────────────────────
void settings_init(void)
{
    // Defaults
    g_sys_cfg.temp_min          = 16;
    g_sys_cfg.temp_max          = 30;
    g_sys_cfg.hvac_mode         = 0;
    g_sys_cfg.ctrl_type         = 0;
    g_sys_cfg.hysteresis_x10    = 10;   // 1.0 °C
    g_sys_cfg.stage_step_x10    = 10;   // 1.0 °C
    g_sys_cfg.sensor_offset_x10 = 0;
    g_sys_cfg.bright_high       = 900;
    g_sys_cfg.bright_low        = 300;
    g_sys_cfg.timeout_s         = INACTIVITY_TIMEOUT_DEFAULT_S;
    g_sys_cfg.modbus_addr       = 1;

    // Open NVS in read-write mode. This will create the namespace if it's the
    // first boot. Opening in read-only (true) would fail on a fresh device.
    s_prefs.begin(NVS_NS, false);
    g_sys_cfg.temp_min          = (int16_t) s_prefs.getInt("temp_min",           g_sys_cfg.temp_min);
    g_sys_cfg.temp_max          = (int16_t) s_prefs.getInt("temp_max",           g_sys_cfg.temp_max);
    g_sys_cfg.hvac_mode         = (uint8_t) s_prefs.getUInt("hvac_mode",         g_sys_cfg.hvac_mode);
    g_sys_cfg.ctrl_type         = (uint8_t) s_prefs.getUInt("ctrl_type",         g_sys_cfg.ctrl_type);
    g_sys_cfg.hysteresis_x10    = (int16_t) s_prefs.getInt("hyst_x10",           g_sys_cfg.hysteresis_x10);
    g_sys_cfg.stage_step_x10    = (int16_t) s_prefs.getInt("stage_x10",          g_sys_cfg.stage_step_x10);
    g_sys_cfg.sensor_offset_x10 = (int16_t) s_prefs.getInt("offset_x10",         g_sys_cfg.sensor_offset_x10);
    g_sys_cfg.bright_high       = (uint16_t)s_prefs.getUInt("bright_high",        g_sys_cfg.bright_high);
    g_sys_cfg.bright_low        = (uint16_t)s_prefs.getUInt("bright_low",         g_sys_cfg.bright_low);
    g_sys_cfg.timeout_s         = (uint8_t) s_prefs.getUInt("timeout_s",          g_sys_cfg.timeout_s);
    g_sys_cfg.modbus_addr       = (uint8_t) s_prefs.getUInt("modbus_addr",        g_sys_cfg.modbus_addr);
    s_prefs.end();

    // Clamp modbus address
    if (g_sys_cfg.modbus_addr < 1 || g_sys_cfg.modbus_addr > 247)
        g_sys_cfg.modbus_addr = 1;

    LOG_INFO("[CFG] Loaded: modbus=%u  bright_h=%u  bright_l=%u  hyst=%.1f",
             g_sys_cfg.modbus_addr,
             g_sys_cfg.bright_high, g_sys_cfg.bright_low,
             g_sys_cfg.hysteresis_x10 / 10.0f);
}

// ── settings_save_dirty ───────────────────────────────────────────────────────
void settings_save_dirty(void)
{
    if (g_dirty_flags == 0) return;

    s_prefs.begin(NVS_NS, false);  // read-write

    if (g_dirty_flags & FLAG_TEMP_MIN)
        s_prefs.putInt("temp_min",    g_sys_cfg.temp_min);
    if (g_dirty_flags & FLAG_TEMP_MAX)
        s_prefs.putInt("temp_max",    g_sys_cfg.temp_max);
    if (g_dirty_flags & FLAG_HVAC_MODE)
        s_prefs.putUInt("hvac_mode",  g_sys_cfg.hvac_mode);
    if (g_dirty_flags & FLAG_CTRL_TYPE)
        s_prefs.putUInt("ctrl_type",  g_sys_cfg.ctrl_type);
    if (g_dirty_flags & FLAG_HYSTERESIS)
        s_prefs.putInt("hyst_x10",    g_sys_cfg.hysteresis_x10);
    if (g_dirty_flags & FLAG_STAGE_STEP)
        s_prefs.putInt("stage_x10",   g_sys_cfg.stage_step_x10);
    if (g_dirty_flags & FLAG_SENSOR_OFFSET)
        s_prefs.putInt("offset_x10",  g_sys_cfg.sensor_offset_x10);
    if (g_dirty_flags & FLAG_BRIGHT_HIGH)
        s_prefs.putUInt("bright_high",g_sys_cfg.bright_high);
    if (g_dirty_flags & FLAG_BRIGHT_LOW)
        s_prefs.putUInt("bright_low", g_sys_cfg.bright_low);
    if (g_dirty_flags & FLAG_TIMEOUT)
        s_prefs.putUInt("timeout_s",  g_sys_cfg.timeout_s);
    if (g_dirty_flags & FLAG_MODBUS_ADDR)
        s_prefs.putUInt("modbus_addr",g_sys_cfg.modbus_addr);

    s_prefs.end();
    LOG_INFO("[CFG] Saved dirty flags: 0x%04X", g_dirty_flags);
    g_dirty_flags = 0;
}

void settings_reset_dirty(void)
{
    g_dirty_flags = 0;
}

// ── Inactivity timeout ────────────────────────────────────────────────────────
void inactivity_reset(void)
{
    // Keep legacy API behavior but ensure any touch restores high brightness.
    hal_backlight_set(g_sys_cfg.bright_high);
    s_last_touch_ms = millis();
}

void inactivity_touch_event(void)
{
    inactivity_reset();
}

void inactivity_set_on_settings(bool on)
{
    s_on_settings    = on;
    s_last_touch_ms  = millis();  // reset timer when entering/leaving settings
}

bool inactivity_on_settings_screen(void)
{
    return s_on_settings;
}

void inactivity_check(void)
{
    if (g_wifi_ap_active) return;   // paused while WiFi AP is active
    if (ui_clean_countdown_active()) return;

    unsigned long elapsed = millis() - s_last_touch_ms;
    if (elapsed >= ((unsigned long)g_sys_cfg.timeout_s * 1000UL)) {
        LOG_INFO("[CFG] Inactivity timeout -> screensaver (dim + Main)");
        inactivity_apply_screensaver();
        s_last_touch_ms = millis();
    }
}

void inactivity_force_timeout(void)
{
    inactivity_apply_screensaver();
    s_last_touch_ms = millis();
}
