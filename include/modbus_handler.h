#pragma once
#include <Arduino.h>
#include <stdint.h>

// ── Register addresses (1-based, FSD §2) ────────────────────────────────────
// Modbus holding register numbers map to 0-based index as: addr - 40001
#define MB_REG_TARGET_TEMP   0   // 40001 – setpoint ×10
#define MB_REG_HVAC_MODE     1   // 40002 – 0=OFF,1=HEAT,2=COOL
#define MB_REG_FAN_SPEED     2   // 40003 – 0=AUTO,1=LOW,2=MID,3=HIGH
#define MB_REG_RELAY_MODE    21  // 40022 – 0=3-speed,1=1-relay
// Weather block: 40030-40046  (0-based: 29-45)
#define MB_REG_WX_WATCHDOG   29  // 40030 – watchdog timestamp flag
#define MB_REG_WX_BASE       30  // 40031 – day-0 data start
#define MB_HREG_COUNT        46  // total holding registers
#define MB_COIL_DND          0   // 00001 – Do Not Disturb
#define MB_COIL_MUR          1   // 00002 – Make Up Room

// ── Input Registers (read-only, 30001+) ─────────────────────────────────────
#define MB_IREG_CURRENT_TEMP  0  // 30001 – current temperature ×10
#define MB_IREG_TARGET_TEMP   1  // 30002 – target temp ×10 (mirror)
#define MB_IREG_RELAY_STATUS  2  // 30003 – relay bits (b0=R1,b1=R2,b2=R3)
#define MB_IREG_UPTIME_LOW    3  // 30004 – uptime seconds (low word)
#define MB_IREG_UPTIME_HIGH   4  // 30005 – uptime seconds (high word)
#define MB_IREG_FREE_HEAP     5  // 30006 – free heap in KB
#define MB_IREG_WINDOW_RAW    6  // 30007 – window sensor raw (0=open,1=closed)
#define MB_IREG_COUNT         7  // total input registers

// ── Discrete Inputs (read-only bits, 10001+) ────────────────────────────────
#define MB_ISTS_WINDOW_CLOSED 0  // 10001 – window sensor (1=closed,0=open)
#define MB_ISTS_SYSTEM_READY  1  // 10002 – system initialized
#define MB_ISTS_HVAC_ACTIVE   2  // 10003 – HVAC relay firing
#define MB_ISTS_WEATHER_VALID 3  // 10004 – weather data not stale
#define MB_ISTS_COUNT         4  // total discrete inputs

// HVAC mode values
#define HVAC_OFF   0
#define HVAC_HEAT  1
#define HVAC_COOL  2

// Fan speed values
#define FAN_AUTO   0
#define FAN_LOW    1
#define FAN_MID    2
#define FAN_HIGH   3

// ── Weather data per day ─────────────────────────────────────────────────────
// Register packing (3 regs per day, 40031-40045):
//   reg+0 = (day_id & 0xFF) | ((icon_id & 0xFF) << 8)
//   reg+1 = temp_high_c10 (signed, ×10 e.g. 250 = 25.0 °C)
//   reg+2 = temp_low_c10  (signed, ×10 e.g. 180 = 18.0 °C)
typedef struct {
    char     day_name[8];      // "Mon", "Tue" … (from day_id lookup)
    uint8_t  icon_id;          // 0=sunny, 1=heating, 2=cooling
    int16_t  temp_high_c10;    // High temperature ×10
    int16_t  temp_low_c10;     // Low  temperature ×10
} wx_day_t;

// Alias used by UI layer (Task 06)
typedef wx_day_t forecast_data_t;

#define WX_MAX_DAYS   5
#define WX_STALE_MS   (12UL * 3600UL * 1000UL)   // 12 h

// ── Shared modbus register mirror ────────────────────────────────────────────
// Read from any module; write only via modbus_set_* helpers to keep Modbus
// slave registers in sync.
typedef struct {
    volatile uint16_t hreg[MB_HREG_COUNT];
    volatile uint16_t ireg[MB_IREG_COUNT];  // input registers (read-only)
    volatile bool     weather_stale;
    volatile unsigned long wx_last_update_ms;
    wx_day_t          wx_days[WX_MAX_DAYS];
} mb_data_t;

extern mb_data_t g_mb;

// ── Public API ───────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

void modbus_init(uint8_t slave_addr);
void modbus_poll(void);               // call every loop()
void modbus_check_watchdog(void);     // call periodically (e.g. every 60 s)
void modbus_update_inputs(void);      // update input registers & discrete inputs

// Helpers to write a holding register AND keep slave mirror in sync
void modbus_set_target_temp(uint16_t val_x10);
void modbus_set_hvac_mode(uint8_t mode);
void modbus_set_fan_speed(uint8_t speed);
void modbus_set_dnd_coil(bool state);
void modbus_set_mur_coil(bool state);

// Getters for read-only status (returns from g_mb.ireg mirror)
uint16_t modbus_get_current_temp(void);   // returns temperature x10
uint16_t modbus_get_relay_status(void);   // returns relay bit field
bool     modbus_get_window_closed(void);  // returns true if window closed
bool     modbus_get_hvac_active(void);    // returns true if HVAC relay on

#ifdef __cplusplus
}
#endif
