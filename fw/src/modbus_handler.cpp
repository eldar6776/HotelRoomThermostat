#include "modbus_handler.h"
#include "settings.h"
#include "debug_logger.h"
#include "hal.h"
#include "hvac.h"  // for hvac_get_room_temp(), hvac_relay_active()
#include <ModbusRTU.h>
#include <Preferences.h>
#include <esp_heap_caps.h>

// ── Global data ───────────────────────────────────────────────────────────────
mb_data_t g_mb;

static ModbusRTU s_mb;

// ── Callbacks ────────────────────────────────────────────────────────────────

// Called by the library on every holding-register write from the master.
// Signature: uint16_t cb(TRegister* reg, uint16_t val)
// Return val to accept it unchanged, or return a different value to override.
static uint16_t cb_hreg_write(TRegister *reg, uint16_t val)
{
    // Always sync g_mb mirror when master writes
    uint16_t reg_index = reg->address.address;
    g_mb.hreg[reg_index] = val;
    
    // NO LOG HERE - would interfere with Modbus response timing on shared Serial!
    // LOG_INFO("[MB] Master write: hreg[%u] = %u", reg_index, val);

    return val;   // accept the written value
}

// ── modbus_init ───────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

void modbus_init(uint8_t slave_addr)
{
    memset(&g_mb, 0, sizeof(g_mb));

    // Modbus shares Serial (UART0) with debug — same baud rate 115200
    // RS485 DE pin: IO41 (dedicated pin, no SPI sharing)
    s_mb.begin(&Serial, PIN_RS485_DE);
    s_mb.slave(slave_addr);

    // Register holding registers block (0-based, MB_HREG_COUNT regs)
    s_mb.addHreg(0, 0, MB_HREG_COUNT);

    // Register input registers (read-only, 30001+)
    s_mb.addIreg(0, 0, MB_IREG_COUNT);

    // Register discrete inputs (read-only bits, 10001+)
    s_mb.addIsts(0, false, MB_ISTS_COUNT);

    // Register coils (DND/MUR)
    s_mb.addCoil(MB_COIL_DND, false, 2);

    // Pre-load default values
    // Load values from NVS settings where available, otherwise use defaults.
    // This ensures that settings persist after a restart.
    g_mb.hreg[MB_REG_TARGET_TEMP] = 220; // Default setpoint 22.0°C, this is a runtime value
    g_mb.hreg[MB_REG_HVAC_MODE]   = g_sys_cfg.hvac_mode; // ✅ LOAD FROM NVS
    g_mb.hreg[MB_REG_FAN_SPEED]   = FAN_AUTO; // Default fan speed

    g_mb.hreg[MB_REG_RELAY_MODE]  = g_sys_cfg.ctrl_type; // ✅ LOAD FROM NVS

    for (int i = 0; i < MB_HREG_COUNT; i++) {
        s_mb.Hreg(i, g_mb.hreg[i]);
    }

    // Register write callback so we react to master writes
    s_mb.onSetHreg(0, cb_hreg_write, MB_HREG_COUNT);

    LOG_INFO("[MB]  Slave init, addr=%u (shared with Serial @115200)", slave_addr);
}

// ── modbus_poll ───────────────────────────────────────────────────────────────
void modbus_poll(void)
{
    s_mb.task();
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void modbus_set_target_temp(uint16_t val_x10)
{
    g_mb.hreg[MB_REG_TARGET_TEMP] = val_x10;
    s_mb.Hreg(MB_REG_TARGET_TEMP, val_x10);
}

void modbus_set_hvac_mode(uint8_t mode)
{
    g_mb.hreg[MB_REG_HVAC_MODE] = mode;
    s_mb.Hreg(MB_REG_HVAC_MODE, mode);
}

void modbus_set_relay_mode(uint8_t mode)
{
    g_mb.hreg[MB_REG_RELAY_MODE] = mode;
    s_mb.Hreg(MB_REG_RELAY_MODE, mode);
}

void modbus_set_fan_speed(uint8_t speed)
{
    LOG_INFO("[MB] modbus_set_fan_speed(%u) called", speed);
    g_mb.hreg[MB_REG_FAN_SPEED] = speed;
    s_mb.Hreg(MB_REG_FAN_SPEED, speed);
    LOG_INFO("[MB] After set: g_mb.hreg[%d]=%u, s_mb.Hreg(%d)=%u", 
             MB_REG_FAN_SPEED, g_mb.hreg[MB_REG_FAN_SPEED], 
             MB_REG_FAN_SPEED, s_mb.Hreg(MB_REG_FAN_SPEED));
}

void modbus_set_dnd_coil(bool state)
{
    s_mb.Coil(MB_COIL_DND, state);
}

void modbus_set_mur_coil(bool state)
{
    s_mb.Coil(MB_COIL_MUR, state);
}

// ── modbus_update_inputs ──────────────────────────────────────────────────────
// Update all input registers and discrete inputs from current system state.
// Call this periodically (e.g., from main loop every 500ms).
void modbus_update_inputs(void)
{
    // Input Register 0: Current temperature x10
    float temp_c = hvac_get_room_temp();
    g_mb.ireg[MB_IREG_CURRENT_TEMP] = (uint16_t)(temp_c * 10.0f);
    s_mb.Ireg(MB_IREG_CURRENT_TEMP, g_mb.ireg[MB_IREG_CURRENT_TEMP]);

    // Input Register 1: Target temperature x10 (mirror from holding reg)
    g_mb.ireg[MB_IREG_TARGET_TEMP] = g_mb.hreg[MB_REG_TARGET_TEMP];
    s_mb.Ireg(MB_IREG_TARGET_TEMP, g_mb.ireg[MB_IREG_TARGET_TEMP]);

    // Input Register 2: Relay status (bit field) — read via I2C expander adapter
    uint16_t relay_bits = 0;
    if (hal_relay_is_on(1)) relay_bits |= (1 << 0);
    if (hal_relay_is_on(2)) relay_bits |= (1 << 1);
    if (hal_relay_is_on(3)) relay_bits |= (1 << 2);
    g_mb.ireg[MB_IREG_RELAY_STATUS] = relay_bits;
    s_mb.Ireg(MB_IREG_RELAY_STATUS, relay_bits);

    // Input Register 3-4: Uptime in seconds (32-bit split)
    uint32_t uptime_sec = millis() / 1000;
    g_mb.ireg[MB_IREG_UPTIME_LOW]  = (uint16_t)(uptime_sec & 0xFFFF);
    g_mb.ireg[MB_IREG_UPTIME_HIGH] = (uint16_t)(uptime_sec >> 16);
    s_mb.Ireg(MB_IREG_UPTIME_LOW, g_mb.ireg[MB_IREG_UPTIME_LOW]);
    s_mb.Ireg(MB_IREG_UPTIME_HIGH, g_mb.ireg[MB_IREG_UPTIME_HIGH]);

    // Input Register 5: Free heap in KB
    uint16_t free_kb = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024;
    g_mb.ireg[MB_IREG_FREE_HEAP] = free_kb;
    s_mb.Ireg(MB_IREG_FREE_HEAP, free_kb);

    // Input Register 6: Window sensor raw (0=open, 1=closed)
    bool window_closed = !hvac_is_window_open();
    g_mb.ireg[MB_IREG_WINDOW_RAW] = window_closed ? 1 : 0;
    s_mb.Ireg(MB_IREG_WINDOW_RAW, g_mb.ireg[MB_IREG_WINDOW_RAW]);

    // Discrete Input 0: Window closed
    s_mb.Ists(MB_ISTS_WINDOW_CLOSED, window_closed);

    // Discrete Input 1: System ready (always true after init)
    s_mb.Ists(MB_ISTS_SYSTEM_READY, true);

    // Discrete Input 2: HVAC active (any relay on)
    bool hvac_active = (relay_bits != 0);
    s_mb.Ists(MB_ISTS_HVAC_ACTIVE, hvac_active);
}

// ── Getter functions ──────────────────────────────────────────────────────────
uint16_t modbus_get_current_temp(void)
{
    return g_mb.ireg[MB_IREG_CURRENT_TEMP];
}

uint16_t modbus_get_relay_status(void)
{
    return g_mb.ireg[MB_IREG_RELAY_STATUS];
}

bool modbus_get_window_closed(void)
{
    return (g_mb.ireg[MB_IREG_WINDOW_RAW] != 0);
}

bool modbus_get_hvac_active(void)
{
    return s_mb.Ists(MB_ISTS_HVAC_ACTIVE);
}

#ifdef __cplusplus
}
#endif
