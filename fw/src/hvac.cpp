#include "hvac.h"
#include "modbus_handler.h"
#include "debug_logger.h"
#include "settings.h"
#include <math.h>
#include <esp_adc_cal.h>

// ── State ─────────────────────────────────────────────────────────────────────
static float     s_room_temp_ema = 20.0f;  // °C
static int       s_setpoint      = 22;     // °C
static uint8_t   s_hvac_mode     = HVAC_OFF;
static uint8_t   s_fan_speed     = FAN_AUTO;
static uint8_t   s_auto_fan_stage = FAN_LOW;
static bool      s_auto_fan_init  = false;
static bool      s_window_open   = false;
static bool      s_sensor_fault  = false;  // true = NTC senzor neispravan
static uint8_t   s_bad_readings  = 0;      // uzastopna loša čitanja
static uint8_t   s_good_readings = 0;      // uzastopna dobra čitanja (za recovery)

// ── ADC kalibracija (ESP-IDF v4 esp_adc_cal) ───────────────────────────────
static esp_adc_cal_characteristics_t s_adc_chars;
static bool                          s_adc_cal_ok = false;

// Deadband state machine
enum db_state_t { DB_IDLE, DB_WAITING };
static db_state_t s_db_state = DB_IDLE;
static unsigned long s_db_start_ms = 0;
static uint8_t   s_db_target_relay = 0;   // relay to turn ON after deadband

// ── NTC: kalibrirani mV → temperatura ────────────────────────────────────────
// v_mv: kalibrirani napon s ADC-a u mV (ESP-IDF adc_cali_raw_to_voltage)
// Vcc NTC djelitelja (NTC_VCC_MV) i ADC referenca su sada eksplicitno odvojeni.
static float adc_to_celsius(int v_mv)
{
    if (v_mv <= 0 || v_mv >= (int)NTC_VCC_MV) return -999.0f;
    float v_out = (float)v_mv / 1000.0f;                    // mV → V
    float v_cc  = NTC_VCC_MV / 1000.0f;                     // 3.3 V
    // Voltage divider: Vout = Vcc * Rntc / (Rseries + Rntc)
    float r_ntc = NTC_SERIES_R * v_out / (v_cc - v_out);
    float kelvin = 1.0f / (log(r_ntc / NTC_NOM_R) / NTC_BETA + 1.0f / NTC_NOM_TEMP);
    return kelvin - 273.15f;
}

// ── Čitanje NTC — Arduino raw + ESP-IDF kalibracija ───────────────────────────
// Vraća kalibrirani napon u mV. Bez kalibracije vraća linearnu procjenu.
static int read_ntc_mv(void)
{
    int raw = analogRead(PIN_NTC);
    if (s_adc_cal_ok) {
        return (int)esp_adc_cal_raw_to_voltage((uint32_t)raw, &s_adc_chars);
    }
    // Fallback: linearna procjena bez kalibracije (3100 mV = full scale za ADC_11db)
    return (int)(raw * 3100 / 4095);
}

// ── Relay control with 100 ms deadband ───────────────────────────────────────
// relay_id: 0=none, 1=RELAY1, 2=RELAY2, 3=RELAY3
static void relay_set_target(uint8_t relay_id)
{
    // Turn all relays OFF first (via I2C expander adapter API)
    hal_relay_all_off();

    if (relay_id == 0) {
        // Clear pending deadband — prevents old relay from re-activating later
        s_db_state        = DB_IDLE;
        s_db_target_relay = 0;
        return;
    }

    // Start deadband timer
    s_db_state        = DB_WAITING;
    s_db_start_ms     = millis();
    s_db_target_relay = relay_id;
}

static void relay_apply_direct(uint8_t relay_id)
{
    switch (relay_id) {
        case 1: hal_relay_set(1, true);  break;
        case 2: hal_relay_set(2, true);  break;
        case 3: hal_relay_set(3, true);  break;
        default: break;
    }
}

static uint8_t get_active_relay_id(void)
{
    if (hal_relay_is_on(1)) return 1;
    if (hal_relay_is_on(2)) return 2;
    if (hal_relay_is_on(3)) return 3;
    return 0;
}

static uint8_t speed_from_abs_error(float abs_error, float step)
{
    if (step <= 0.0f) {
        return FAN_LOW;
    }

    if (abs_error < step) {
        return FAN_LOW;
    }
    if (abs_error < 2.0f * step) {
        return FAN_MID;
    }
    return FAN_HIGH;
}

static uint8_t auto_speed_schmitt(float abs_error, float step, float fan_diff)
{
    float t1 = step;
    float t2 = 2.0f * step;

    switch (s_auto_fan_stage) {
        case FAN_LOW:
            if (abs_error >= (t1 + fan_diff)) {
                s_auto_fan_stage = FAN_MID;
            }
            break;
        case FAN_MID:
            if (abs_error >= (t2 + fan_diff)) {
                s_auto_fan_stage = FAN_HIGH;
            } else if (abs_error <= (t1 - fan_diff)) {
                s_auto_fan_stage = FAN_LOW;
            }
            break;
        case FAN_HIGH:
            if (abs_error <= (t2 - fan_diff)) {
                s_auto_fan_stage = FAN_MID;
            }
            break;
        default:
            s_auto_fan_stage = FAN_LOW;
            break;
    }

    return s_auto_fan_stage;
}

// ── Pick relay based on mode / fan speed ────────────────────────────────────
// relay_mode 0 = 3-speed:  relay1=low, relay2=mid, relay3=high, auto=low
// relay_mode 1 = 1-relay : relay1 = on/off
static uint8_t pick_relay(float error, float hyst)
{
    bool relay_mode_1relay = (g_mb.hreg[MB_REG_RELAY_MODE] == 1);

    if (s_hvac_mode == HVAC_OFF) return 0;

    if (relay_mode_1relay) {
        return 1;  // single relay only
    }

    uint8_t effective_speed = s_fan_speed;
    if (s_fan_speed == FAN_AUTO) {
        float abs_error = fabsf(error);
        float step = g_sys_cfg.stage_step_x10 / 10.0f;

        // BUGFIX: Fan stage Schmitt širina ne smije biti cijela termostatska histereza.
        // Ako bi bila (npr. 1.0°C), termostat bi se zaglavio u MID ili HIGH brzini
        // jer bi mu trebalo previše da spusti brzinu. Koristimo 20% od koraka (npr. 0.2°C).
        float fan_diff = step * 0.2f;

        if (!s_auto_fan_init) {
            s_auto_fan_stage = speed_from_abs_error(abs_error, step);
            s_auto_fan_init = true;
        }

        effective_speed = auto_speed_schmitt(abs_error, step, fan_diff);
    } else {
        s_auto_fan_init = false;
    }

    // 3-speed fan
    switch (effective_speed) {
        case FAN_LOW:  return 1;
        case FAN_MID:  return 2;
        case FAN_HIGH: return 3;
        default:       return 1;
    }
}

// ── hvac_init ────────────────────────────────────────────────────────────────
void hvac_init(void)
{
    // ── ADC init (Arduino) + ESP-IDF kalibracija (ADC1_CH0 = IO1 = PIN_NTC) ─────
    analogSetAttenuation(ADC_11db);
    analogSetPinAttenuation(PIN_NTC, ADC_11db);

    // esp_adc_cal čita fabrički upisane eFuse kalibracijske točke na ESP32-S3.
    // Ovo kompenzuje chip-to-chip varijancu i termalni drift interne reference.
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 3300, &s_adc_chars);
    if (cal_type == ESP_ADC_CAL_VAL_EFUSE_TP || cal_type == ESP_ADC_CAL_VAL_EFUSE_TP_FIT) {
        s_adc_cal_ok = true;
        LOG_INFO("[HVAC] ADC kalibracija OK (eFuse Two Point, type=%d)", (int)cal_type);
    } else if (cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        s_adc_cal_ok = true;
        LOG_INFO("[HVAC] ADC kalibracija OK (eFuse Vref, type=%d)", (int)cal_type);
    } else {
        s_adc_cal_ok = false;
        LOG_ERROR("[HVAC] ADC kalibracija NIJE dostupna (type=%d) — fallback na linearnu procjenu", (int)cal_type);
    }

    // Read initial room temperature
    int v_mv = read_ntc_mv();
    float t = adc_to_celsius(v_mv);
    if (t > -50.0f && t < 85.0f) {
        s_room_temp_ema = t;
    }

    // Load setpoint from shared Modbus register
    s_setpoint   = g_mb.hreg[MB_REG_TARGET_TEMP] / 10;
    s_hvac_mode  = (uint8_t)g_mb.hreg[MB_REG_HVAC_MODE];
    s_fan_speed  = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];

    LOG_INFO("[HVAC] Init OK  setpoint=%d°C  mode=%u  fan=%u",
             s_setpoint, s_hvac_mode, s_fan_speed);
}

// ── hvac_update ──────────────────────────────────────────────────────────────
void hvac_update(void)
{
    // 0. Sinkroniziraj stanje s Modbus registrima (Single Source of Truth)
    int new_setpoint   = g_mb.hreg[MB_REG_TARGET_TEMP] / 10;
    if (new_setpoint < g_sys_cfg.temp_min) new_setpoint = g_sys_cfg.temp_min;
    if (new_setpoint > g_sys_cfg.temp_max) new_setpoint = g_sys_cfg.temp_max;
    if (g_mb.hreg[MB_REG_TARGET_TEMP] != new_setpoint * 10) {
        modbus_set_target_temp(new_setpoint * 10);
    }
    uint8_t new_mode   = (uint8_t)g_mb.hreg[MB_REG_HVAC_MODE];
    uint8_t new_fan    = (uint8_t)g_mb.hreg[MB_REG_FAN_SPEED];

    // BUGFIX: Ako Modbus master ili prvi boot promijeni stanje moda, brzine,
    // ili SETPOINTA, obavezno resetuj auto-fan inicijalizaciju
    if (s_hvac_mode != new_mode || s_fan_speed != new_fan || s_setpoint != new_setpoint) {
        s_auto_fan_init = false;
        // Opcionalni Safe-Off ako se mijenja sam HVAC mod:
        if (s_hvac_mode != new_mode) relay_set_target(0);
        
        if (s_setpoint != new_setpoint && g_sys_cfg.target_temp != new_setpoint) {
            g_sys_cfg.target_temp = new_setpoint;
            g_dirty_flags |= FLAG_TARGET_TEMP;
            settings_schedule_save();
        }
    }

    s_setpoint   = new_setpoint;
    s_hvac_mode  = new_mode;
    s_fan_speed  = new_fan;

    // 1. Read NTC with EMA filter
    int   raw_adc  = analogRead(PIN_NTC);
    int   v_mv     = s_adc_cal_ok
                         ? (int)esp_adc_cal_raw_to_voltage((uint32_t)raw_adc, &s_adc_chars)
                         : (int)(raw_adc * 3100 / 4095);
    float v_out    = v_mv / 1000.0f;
    float v_cc     = NTC_VCC_MV / 1000.0f;
    float r_ntc    = (v_out > 0.0f && v_out < v_cc)
                         ? NTC_SERIES_R * v_out / (v_cc - v_out)
                         : -1.0f;
    float t        = adc_to_celsius(v_mv);
    bool   valid   = (t > -50.0f && t < 85.0f && r_ntc > 0.0f);

    if (valid) {
        s_bad_readings = 0;
        s_good_readings++;
        s_room_temp_ema = EMA_ALPHA * t + (1.0f - EMA_ALPHA) * s_room_temp_ema;

        if (s_sensor_fault && s_good_readings >= 3) {
            s_sensor_fault = false;
            LOG_INFO("[HVAC] Sensor fault CLEARED — %u good readings", s_good_readings);
        }
    } else {
        s_good_readings = 0;
        if (s_bad_readings < 255) s_bad_readings++;

        if (s_bad_readings >= 5) {
            if (!s_sensor_fault) {
                s_sensor_fault = true;
                LOG_ERROR("[HVAC] Sensor FAULT detected — %u bad readings, t=%.1f, r_ntc=%.0f",
                          s_bad_readings, t, r_ntc);
            }
        }
    }

    // 2. Window sensor (Active LOW — read via I2C expander)
    bool window_now_open = hal_window_is_open();
    if (window_now_open != s_window_open) {
        LOG_INFO("[HVAC] Window state changed: %s → %s",
                 s_window_open ? "OPEN" : "CLOSED",
                 window_now_open ? "OPEN" : "CLOSED");
        s_window_open = window_now_open;
    }

    uint8_t desired_relay = 0;

    if (s_sensor_fault || (s_window_open && s_hvac_mode != HVAC_OFF)) {
        desired_relay = 0;
    } else if (s_db_state == DB_WAITING) {
        // 3. Deadband guard — thermostat logic is blocked while a relay switch is in progress.
        return;
    } else {
        // 4. Thermostat logic (simple hysteresis from settings)
        float compensated_room_temp = s_room_temp_ema + (g_sys_cfg.sensor_offset_x10 / 10.0f);
        float hyst = g_sys_cfg.hysteresis_x10 / 10.0f;
        float error = (float)s_setpoint - compensated_room_temp;

        switch (s_hvac_mode) {
            case HVAC_HEAT:
                if (error > (hyst / 2.0f))
                    desired_relay = pick_relay(error, hyst);
                else if (error < -(hyst / 2.0f))
                    desired_relay = 0;
                else
                    // hysteresis band — keep current state
                    desired_relay = (hal_relay_is_on(1) || hal_relay_is_on(2) || hal_relay_is_on(3))
                                    ? pick_relay(error, hyst) : 0;
                break;
            case HVAC_COOL:
                if (error < -(hyst / 2.0f))
                    desired_relay = pick_relay(error, hyst);
                else if (error > (hyst / 2.0f))
                    desired_relay = 0;
                else
                    desired_relay = (hal_relay_is_on(1) || hal_relay_is_on(2) || hal_relay_is_on(3))
                                    ? pick_relay(error, hyst) : 0;
                break;
            case HVAC_OFF:
            default:
                desired_relay = 0;
                break;
        }
    }

    // Only change if different from current state (avoids continuous deadband triggering)
    bool any_on = hal_relay_is_on(1) || hal_relay_is_on(2) || hal_relay_is_on(3);
    bool want_on = (desired_relay != 0);
    uint8_t active_relay = get_active_relay_id();

    // Safety: Force hardware sync every 5 seconds even if state hasn't changed
    static unsigned long s_last_sync_ms = 0;
    if (millis() - s_last_sync_ms >= 5000) {
        s_last_sync_ms = millis();
        hal_relay_sync();
    }

    // Safety: Check for relay fault and enter safe-off state
    if (hal_relay_has_fault()) {
        LOG_ERROR("[HVAC] Relay FAULT detected — forcing safe-off");
        relay_set_target(0);
        return;
    }

    if (want_on != any_on || (want_on && desired_relay != active_relay)) {
        LOG_DEBUG("HVAC state change: current_relay=%d, desired_relay=%d", active_relay, desired_relay);
        relay_set_target(desired_relay);
    }
}

// ── Getters ──────────────────────────────────────────────────────────────────
float hvac_get_room_temp(void)  { return s_room_temp_ema; }
bool  hvac_is_window_open(void)    { return s_window_open; }
bool  hvac_temp_sensor_fault(void) { return s_sensor_fault; }

// ── hvac_deadband_tick ────────────────────────────────────────────────────────
// Must be called every loop() iteration.  Fires the pending relay exactly
// RELAY_DEADBAND_MS after the switch request — independently of the 500 ms
// hvac_update() period so the constant actually means what it says.
void hvac_deadband_tick(void)
{
    if (s_db_state != DB_WAITING) return;
    if ((millis() - s_db_start_ms) >= RELAY_DEADBAND_MS) {
        relay_apply_direct(s_db_target_relay);
        LOG_DEBUG("Deadband finished, relay %d is ON", s_db_target_relay);
        s_db_state = DB_IDLE;
    }
}

// ── Setters (called from UI callbacks) ───────────────────────────────────────
void hvac_set_setpoint(int temp_c)
{
    if (temp_c < g_sys_cfg.temp_min) temp_c = g_sys_cfg.temp_min;
    if (temp_c > g_sys_cfg.temp_max) temp_c = g_sys_cfg.temp_max;
    s_setpoint = temp_c;
    
    s_auto_fan_init = false; // Resetuj izračun brzine čim korisnik zavrti arc slider
    modbus_set_target_temp((uint16_t)(temp_c * 10));

    // Mark setpoint dirty — actual NVS write deferred 3 s to protect Flash
    g_sys_cfg.target_temp = (int16_t)temp_c;
    g_dirty_flags |= FLAG_TARGET_TEMP;
    settings_schedule_save();
}

void hvac_set_mode(uint8_t mode)
{
    s_hvac_mode = mode;
    s_auto_fan_init = false;
    modbus_set_hvac_mode(mode);
    relay_set_target(0);  // safe-off on mode change
}

void hvac_set_fan_speed(uint8_t spd)
{
    LOG_DEBUG("hvac_set_fan_speed called with: %u", spd);
    s_fan_speed = spd;
    s_auto_fan_init = false;
    modbus_set_fan_speed(spd);
    LOG_DEBUG("After modbus_set_fan_speed, g_mb.hreg[MB_REG_FAN_SPEED] = %u", g_mb.hreg[MB_REG_FAN_SPEED]);
    relay_set_target(0);  // re-evaluate in next update cycle
}
