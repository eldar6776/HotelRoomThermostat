#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "hal.h"
#include "settings.h"
#include "modbus_handler.h"
#include "hvac.h"
#include "debug_logger.h"

extern "C" {
    #include "../lvgl/ui.h"
    #include "../lvgl/ui_events.h"
    // Add event handlers for DND/MUR
    extern lv_obj_t * ui_ButtonDnd;
    extern lv_obj_t * ui_ButtonMur;
    void action_dnd_toggled(lv_event_t * e);
    void action_mur_toggled(lv_event_t * e);
    void settings3_loaded_cb(lv_event_t *e);
}

// ── Timing constants ──────────────────────────────────────────────────────────
#define HVAC_UPDATE_MS         500UL   // NTC read + relay logic period
#define MODBUS_WATCHDOG_MS   60000UL   // weather watchdog check period
#define CLOCK_UPDATE_MS       1000UL   // clock label refresh period
#define INACTIVITY_CHECK_MS   1000UL   // inactivity poll period

// ── Timestamps ────────────────────────────────────────────────────────────────
static unsigned long t_hvac      = 0;
static unsigned long t_watchdog  = 0;
static unsigned long t_clock     = 0;
static unsigned long t_inact     = 0;

// ── Clock helpers ─────────────────────────────────────────────────────────────
// RTC is not set in this firmware — show uptime-based time as placeholder
// (NTP sync is a future enhancement)
static void update_clock_label(void)
{
    unsigned long s = millis() / 1000;
    unsigned int  h = (s / 3600) % 24;
    unsigned int  m = (s / 60)   % 60;
    // unsigned int  sec = s         % 60; // Sekunde uklonjene
    lv_label_set_text_fmt(ui_LabelClock, "%02u:%02u", h, m);
}

// ── Room temperature → UI ────────────────────────────────────────────────────
static void update_temp_labels(void)
{
    float t = hvac_get_room_temp()
              + (g_sys_cfg.sensor_offset_x10 / 10.0f);
    char temp_buf[32]; // Buffer for formatted strings

    // Main screen label (one line)
    snprintf(temp_buf, sizeof(temp_buf), "Innen: %.1f°C", (double)t);
    lv_label_set_text(ui_LabelCurrentTemp, temp_buf);

    // Thermostat screen label (two lines)
    snprintf(temp_buf, sizeof(temp_buf), "Innen:\n%.1f°C", (double)t);
    lv_label_set_text(ui_LabelRoomTemp, temp_buf);

    // Window open warning
    if (hvac_is_window_open()) {
//        lv_label_set_text(ui_LabelWeatherTodayDesc, "WINDOW OPEN");
//        lv_obj_clear_flag(ui_LabelWeatherTodayDesc, LV_OBJ_FLAG_HIDDEN);
    } else {
//        lv_obj_add_flag(ui_LabelWeatherTodayDesc, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── HVAC mode images ─────────────────────────────────────────────────────────
static void update_hvac_icons(void)
{
    uint8_t mode = g_mb.hreg[MB_REG_HVAC_MODE];
    lv_obj_add_flag(ui_ImageHeatStatus, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ImageCoolStatus, LV_OBJ_FLAG_HIDDEN);
    if (mode == HVAC_HEAT)
        lv_obj_clear_flag(ui_ImageHeatStatus, LV_OBJ_FLAG_HIDDEN);
    else if (mode == HVAC_COOL)
        lv_obj_clear_flag(ui_ImageCoolStatus, LV_OBJ_FLAG_HIDDEN);
}

// ── Outside temperature → UI ───────────────────────────────────────────────────
// Placeholder for future weather integration
static void update_outside_temp_label(void)
{
    // For now, this is a static placeholder.
    // In the future, this will be driven by Modbus weather data.
    // TODO: Replace with lv_label_set_text_fmt(ui_LabelOutdoorTemp, "Aussen: %.0f°C", outside_temp);
    lv_label_set_text(ui_LabelOutdoorTemp, "Aussen: 18°C");
}

// ── setup ────────────────────────────────────────────────────────────────────
void setup(void)
{
    Serial.begin(115200);
    delay(300);
    
    LOG_INFO("=== Hotel Room Thermostat ===");
    LOG_INFO("CPU: %lu MHz   PSRAM: %.1f MB free",
             getCpuFrequencyMhz(),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1048576.0f);

    // 1. Load NVS configuration
    settings_init();
    LOG_INFO("[MAIN] Settings loaded");

    // 2. Initialise all hardware + LVGL
    board_hal_init();
    LOG_INFO("[MAIN] HAL init done");

    // 3. Apply stored backlight level
    hal_backlight_set(g_sys_cfg.bright_high);
    LOG_INFO("[MAIN] Backlight set to %u", g_sys_cfg.bright_high);

    // 4. Start LVGL UI (SquareLine generated)
    LOG_INFO("[MAIN] Starting UI init...");
    ui_init();
    LOG_INFO("[MAIN] UI init done");
    LOG_INFO("[MAIN] Free heap: %.1f KB  PSRAM: %.1f MB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024.0f,
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1048576.0f);

    // Try one lv_timer_handler call here to catch crash early
    LOG_INFO("[MAIN] First lv_timer_handler call...");
    lv_timer_handler();
    LOG_INFO("[MAIN] First lv_timer_handler done");

    // 5. Modbus RTU slave
    modbus_init(g_sys_cfg.modbus_addr);

    // 6. HVAC / thermostat logic (uses g_sys_cfg and g_mb)
    hvac_init();

    // 7. Seed arc setpoint from NVS & set initial temps
    int sp = g_mb.hreg[MB_REG_TARGET_TEMP] / 10;
    lv_arc_set_value(ui_ArcTemp, sp);
    lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", sp);
    
    // Set initial fan speed label from Modbus register
    uint8_t fan_speed = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];
    switch (fan_speed) {
        case FAN_AUTO: lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
        case FAN_LOW:  lv_label_set_text(ui_LabelFanStatus, "Low");  break;
        case FAN_MID:  lv_label_set_text(ui_LabelFanStatus, "Mid");  break;
        case FAN_HIGH: lv_label_set_text(ui_LabelFanStatus, "High"); break;
        default:       lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
    }
    
    // 9. Register Settings3 slider init callback (for swipe navigation)
    lv_obj_add_event_cb(ui_Settings3, settings3_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    // Also init on first screen creation
    settings3_loaded_cb(NULL);

    // 10. DND/MUR button logic
    lv_obj_add_flag(ui_ButtonDnd, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_flag(ui_ButtonMur, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(ui_ButtonDnd, action_dnd_toggled, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_ButtonMur, action_mur_toggled, LV_EVENT_VALUE_CHANGED, NULL);
    LOG_INFO("[MAIN] DND/MUR event handlers attached.");

    LOG_INFO("[MAIN] Setup complete");
}

// ── loop ─────────────────────────────────────────────────────────────────────
void loop(void)
{
    unsigned long now = millis();

    // LVGL task handler (must be called frequently)
    lv_timer_handler();

    // Modbus poll
    modbus_poll();

    // HVAC update (NTC + relay)
    if (now - t_hvac >= HVAC_UPDATE_MS) {
        t_hvac = now;
        hvac_update();
        update_temp_labels();
        update_hvac_icons();
        // Update Modbus input registers with current system state
        modbus_update_inputs();
    }

    // Weather watchdog check
    if (now - t_watchdog >= MODBUS_WATCHDOG_MS) {
        t_watchdog = now;
        modbus_check_watchdog();
    }

    // Clock refresh
    if (now - t_clock >= CLOCK_UPDATE_MS) {
        t_clock = now;
        update_clock_label();
    }

    // Inactivity check (settings screens)
    if (now - t_inact >= INACTIVITY_CHECK_MS) {
        t_inact = now;
        inactivity_check();
    }

    // Small yield to prevent WDT reset
    delay(2);
}
