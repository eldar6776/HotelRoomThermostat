#pragma once
#include <Arduino.h>
#include "hal.h"

// ── NTC configuration (100k B3950) ──────────────────────────────────────────
#define NTC_BETA          3950.0f
#define NTC_NOM_R         100000.0f   // 100 kΩ at 25 °C
#define NTC_NOM_TEMP      298.15f     // 25 °C in Kelvin
#define NTC_SERIES_R      100000.0f   // series resistor 100 kΩ
#define NTC_VCC_MV        3300.0f     // napon gornjeg kraka NTC djelitelja (3.3V LDO), u mV
#define ADC_MAX_VAL       4095.0f
#define EMA_ALPHA         0.1f        // exponential moving average factor

// Temperature limits (°C)
#define TEMP_SETPOINT_MIN  10
#define TEMP_SETPOINT_MAX  40

// Relay deadband delay (ms)
#define RELAY_DEADBAND_MS  100

// ── Public API ───────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif
void hvac_init(void);
void hvac_update(void);               // call every loop() iteration

float hvac_get_room_temp(void);       // current measured temp (°C)
bool  hvac_is_window_open(void);

// Called from UI callbacks
void hvac_set_setpoint(int temp_c);   // clamps to [10..40]
void hvac_set_mode(uint8_t mode);     // HVAC_OFF/HEAT/COOL
void hvac_set_fan_speed(uint8_t spd); // FAN_AUTO/LOW/MID/HIGH
#ifdef __cplusplus
}
#endif
