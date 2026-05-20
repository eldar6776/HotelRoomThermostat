#include <Arduino.h>
#include <esp_heap_caps.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include <lvgl.h>

#include "hal.h"
#include "settings.h"
#include "modbus_handler.h"
#include "hvac.h"
#include "debug_logger.h"
#include "wifi_manager.h"

extern "C" {
    #include "../lvgl/ui.h"
    #include "../lvgl/ui_events.h"
    // Add event handlers for DND/MUR
    extern lv_obj_t * ui_ButtonDnd;
    extern lv_obj_t * ui_ButtonMur;
    void action_dnd_toggled(lv_event_t * e);
    void action_mur_toggled(lv_event_t * e);
    void settings3_loaded_cb(lv_event_t *e);
    void action_arc_temp_changed(lv_event_t * e);
}

// Coil mirrors from modbus_handler
extern bool g_mb_coil_dnd;
extern bool g_mb_coil_mur;
// Rename local shadows to use extern names
#define s_mb_coil_dnd g_mb_coil_dnd
#define s_mb_coil_mur g_mb_coil_mur

// ── Timing constants ──────────────────────────────────────────────────────────
#define HVAC_UPDATE_MS         500UL   // NTC read + relay logic period
#define CLOCK_UPDATE_MS       1000UL   // clock label refresh period
#define INACTIVITY_CHECK_MS   1000UL   // inactivity poll period

// Timezone constant for Sarajevo (CET/CEST with DST transitions)
#define SARAJEVO_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

// ── Timestamps ────────────────────────────────────────────────────────────────
static unsigned long t_hvac      = 0;
static unsigned long t_clock     = 0;
static unsigned long t_inact     = 0;
static unsigned long s_boot_ms   = 0;

// ── Time Sync State Machine ───────────────────────────────────────────────────
static bool s_is_clock_set = false;
static unsigned long s_last_sync_ms = 0;
static bool s_initial_sync_pending = true;

extern "C" void on_time_synced(const char* source) {
    s_is_clock_set = true;
    s_last_sync_ms = millis();
    s_initial_sync_pending = false; // No need for initial sync anymore
    LOG_INFO("[TIME] Clock synchronized via %s", source);
}

void try_ntp_sync() {
    if (g_wifi_ap_active) {
        LOG_INFO("[NTP] Sync skipped, WiFi AP is active.");
        return;
    }

    if (!wifi_manager_has_credentials()) {
        LOG_INFO("[NTP] Sync skipped, no WiFi credentials saved.");
        return;
    }

    LOG_INFO("[NTP] Starting NTP sync attempt...");
    
    bool was_screensaver = inactivity_is_screensaver_active();
    if (getCpuFrequencyMhz() < 160) {
        setCpuFrequencyMhz(240);
        LOG_INFO("[NTP] CPU clock restored to 240MHz for WiFi.");
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin();

    unsigned long connect_start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (millis() - connect_start > 20000) { // 20s connect timeout
            LOG_ERROR("[NTP] WiFi connection timed out.");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            if (was_screensaver) {
                setCpuFrequencyMhz(80);
                LOG_INFO("[NTP] CPU clock returned to 80MHz.");
            }
            return;
        }
    }
    LOG_INFO("[NTP] WiFi connected: %s", WiFi.localIP().toString().c_str());

    configTzTime(SARAJEVO_TZ, "pool.ntp.org", "time.nist.gov");
    
    unsigned long ntp_start = millis();
    while (time(nullptr) < 1672531200) { // Check if time is later than 2023-01-01
        delay(100);
        if (millis() - ntp_start > 10000) { // 10s NTP timeout
            LOG_ERROR("[NTP] NTP time sync timed out.");
            break;
        }
    }

    if (time(nullptr) > 1672531200) {
        on_time_synced("NTP");
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    LOG_INFO("[NTP] WiFi disconnected, sync process finished.");

    if (was_screensaver) {
        setCpuFrequencyMhz(80);
        LOG_INFO("[NTP] CPU clock returned to 80MHz.");
    }
}

// ── Clock helpers ─────────────────────────────────────────────────────────────
static void update_clock_label(void)
{
    if (!s_is_clock_set) {
        unsigned long s = millis() / 1000;
        unsigned int  h = (s / 3600) % 24;
        unsigned int  m = (s / 60)   % 60;
        lv_label_set_text_fmt(ui_LabelClock, "%02u:%02u", h, m);
    } else {
        time_t now_time = time(NULL);
        struct tm *t = localtime(&now_time);
        lv_label_set_text_fmt(ui_LabelClock, "%02d:%02d", t->tm_hour, t->tm_min);
    }
}

// ── Room temperature → UI ────────────────────────────────────────────────────
static void update_temp_labels(void)
{
    float t = hvac_get_room_temp()
              + (g_sys_cfg.sensor_offset_x10 / 10.0f);
    char temp_buf[32]; // Buffer for formatted strings

    // Main screen label (one line, 5 spaces)
    snprintf(temp_buf, sizeof(temp_buf), "Innen:     %.0f°C", roundf(t));
    lv_label_set_text(ui_LabelCurrentTemp, temp_buf);

    // Thermostat screen label (one line, no spaces)
    snprintf(temp_buf, sizeof(temp_buf), "Innen:  %.0f°C", roundf(t));
    lv_label_set_text(ui_LabelRoomTemp, temp_buf);

    // Window open warning — no UI element currently assigned
    if (hvac_is_window_open()) {
    } else {
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

// ── Thermostat widget sync (Modbus → GUI) ────────────────────────────────────
// Detects when Modbus registers change (master write or boot) and updates
// Arc, fan label, and DND/MUR buttons. Called every HVAC_UPDATE_MS period.
static uint16_t s_last_displayed_setpoint = 0xFFFF;  // sentinel: force first update
static uint8_t  s_last_displayed_fan      = 0xFF;
static bool     s_last_displayed_dnd      = false;
static bool     s_last_displayed_mur      = false;

static void update_thermostat_widgets(void)
{
    // ── Setpoint Arc & Label ──────────────────────────────────────────────────
    // Skip Modbus override while the user is actively dragging the arc.
    // LV_STATE_PRESSED is set by LVGL for the entire duration of the touch.
    uint16_t mb_setpoint_x10 = g_mb.hreg[MB_REG_TARGET_TEMP];
    if (!lv_obj_has_state(ui_ArcTemp, LV_STATE_PRESSED) &&
        mb_setpoint_x10 != s_last_displayed_setpoint) {
        s_last_displayed_setpoint = mb_setpoint_x10;
        int sp = (int)(mb_setpoint_x10 / 10);
        // Remove only our specific callback to avoid silently discarding any
        // future callbacks registered on this object.
        lv_obj_remove_event_cb(ui_ArcTemp, action_arc_temp_changed);
        lv_arc_set_value(ui_ArcTemp, sp);
        lv_obj_add_event_cb(ui_ArcTemp, action_arc_temp_changed, LV_EVENT_VALUE_CHANGED, NULL);
        lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", sp);
    }

    // ── Fan Speed Label ───────────────────────────────────────────────────────
    uint8_t mb_fan = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];
    if (mb_fan != s_last_displayed_fan) {
        s_last_displayed_fan = mb_fan;
        switch (mb_fan) {
            case FAN_AUTO: lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
            case FAN_LOW:  lv_label_set_text(ui_LabelFanStatus, "Low");  break;
            case FAN_MID:  lv_label_set_text(ui_LabelFanStatus, "Mid");  break;
            case FAN_HIGH: lv_label_set_text(ui_LabelFanStatus, "High"); break;
            default:       lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
        }
    }

    // ── DND / MUR Coils ───────────────────────────────────────────────────────
    bool mb_dnd = s_mb_coil_dnd;
    bool mb_mur = s_mb_coil_mur;
    if (mb_dnd != s_last_displayed_dnd) {
        s_last_displayed_dnd = mb_dnd;
        if (mb_dnd)
            lv_obj_add_state(ui_ButtonDnd, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(ui_ButtonDnd, LV_STATE_CHECKED);
    }
    if (mb_mur != s_last_displayed_mur) {
        s_last_displayed_mur = mb_mur;
        if (mb_mur)
            lv_obj_add_state(ui_ButtonMur, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(ui_ButtonMur, LV_STATE_CHECKED);
    }
}

// ── Outside temperature → UI ───────────────────────────────────────────────────
// Placeholder for future weather integration
static void update_outside_temp_label(void)
{
    if (modbus_has_outside_temp()) {
        lv_obj_clear_flag(ui_LabelOutdoorTemp, LV_OBJ_FLAG_HIDDEN);
        // Cast uint16_t -> int16_t -> int da bi ispravno očitali negativne temperature (npr. -5°C)
        int out_temp = (int)(int16_t)g_mb.hreg[MB_REG_OUTSIDE_TEMP];
        char temp_buf[32];
        snprintf(temp_buf, sizeof(temp_buf), "Aussen:     %d°C", out_temp);
        lv_label_set_text(ui_LabelOutdoorTemp, temp_buf);
    } else {
        lv_obj_add_flag(ui_LabelOutdoorTemp, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── setup ────────────────────────────────────────────────────────────────────
void setup(void)
{
    Serial.begin(115200);
    delay(300);
    
    s_boot_ms = millis();

    LOG_INFO("=== Hotel Room Thermostat ===");
    LOG_INFO("CPU: %lu MHz   PSRAM: %.1f MB free",
             getCpuFrequencyMhz(),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1048576.0f);

    // 1. Load NVS configuration
    settings_init();
    LOG_INFO("[MAIN] Settings loaded");

    // 1.5 Init WiFi Manager
    wifi_manager_init();

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

    // 7. Seed arc setpoint from NVS — use obj flag to suppress VALUE_CHANGED event
    int sp = g_mb.hreg[MB_REG_TARGET_TEMP] / 10;
    lv_obj_remove_event_cb(ui_ArcTemp, NULL);  // detach all callbacks temporarily
    lv_arc_set_value(ui_ArcTemp, sp);          // set value without triggering hvac_set_setpoint
    lv_obj_add_event_cb(ui_ArcTemp, action_arc_temp_changed, LV_EVENT_VALUE_CHANGED, NULL);  // re-attach
    lv_label_set_text_fmt(ui_LabelTargetTemp, "%d°", sp);
    // Seed the mirror so update_thermostat_widgets() skips on first loop iteration
    s_last_displayed_setpoint = (uint16_t)(sp * 10);
    
    // Set initial fan speed label from Modbus register
    uint8_t fan_speed = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];
    switch (fan_speed) {
        case FAN_AUTO: lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
        case FAN_LOW:  lv_label_set_text(ui_LabelFanStatus, "Low");  break;
        case FAN_MID:  lv_label_set_text(ui_LabelFanStatus, "Mid");  break;
        case FAN_HIGH: lv_label_set_text(ui_LabelFanStatus, "High"); break;
        default:       lv_label_set_text(ui_LabelFanStatus, "Auto"); break;
    }
    s_last_displayed_fan = fan_speed;  // seed mirror
    
    // 9. Initialize Settings3 controls on first screen creation.
    // Screen-loaded callback is already attached in generated LVGL code.
    settings3_loaded_cb(NULL);

    // 10. DND/MUR handlers are attached in generated LVGL screen code.
    LOG_INFO("[MAIN] DND/MUR event handlers ready.");

    LOG_INFO("[MAIN] Setup complete");
}

// ── loop ─────────────────────────────────────────────────────────────────────
void loop(void)
{
    unsigned long now = millis();

    // Deadband relay timer — runs every loop() for accurate RELAY_DEADBAND_MS timing
    hvac_deadband_tick();

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
        update_thermostat_widgets();  // sync Modbus→GUI for setpoint, fan, DND/MUR
        // Update Modbus input registers with current system state
        modbus_update_inputs();
        
        modbus_check_outside_temp_timeout();
        update_outside_temp_label();
    }

    // Time Sync Logic
    if (s_initial_sync_pending && (now - s_boot_ms > 15000UL)) {
        LOG_INFO("[TIME] 15s boot delay passed, attempting initial NTP sync.");
        s_initial_sync_pending = false; // Attempt only once
        try_ntp_sync();
    }

    if (s_is_clock_set && (now - s_last_sync_ms > 3600000UL)) { // 60 minutes
        LOG_INFO("[TIME] 60min since last sync, attempting fallback NTP sync.");
        try_ntp_sync();
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

    // Obrada WebServer zahtjeva za OTA (samo kad je AP uključen)
    if (g_wifi_ap_active) {
        wifi_manager_tick();
    }

    // Deferred NVS save — fires once, 3 s after the last settings change
    settings_tick();

    // Small yield to prevent WDT reset
    delay(2);
}
