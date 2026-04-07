#pragma once
#include <Arduino.h>

// ── ESP32-S3 direktni pinovi (FSD §1 — ažurirano za I2C expander) ───────────

// I2C magistrala za GPIO expander (I2C1 — nezavisna od Touch I2C0)
#define PIN_I2C_SDA         2    // IO2  - I2C1 SDA → PCF8574AN / PCA9554
#define PIN_I2C_SCL         40   // IO40 - I2C1 SCL → PCF8574AN / PCA9554

// RS485 Driver Enable — premješten sa IO48 na IO41 (dedicated pin)
#define PIN_RS485_DE        41   // IO41 - THVD1500 Driver Enable

// NTC termistor — ostaje nepromijenjen
#define PIN_NTC             1    // IO1  - NTC 100k B3950 (ADC1_CH0)

// LCD Backlight — ostaje nepromijenjen
#define PIN_LCD_BL          38   // IO38 - LCD Backlight (PWM)

// Touch (I2C0 — GT911) — ostaje nepromijenjen
#define PIN_TOUCH_SDA       19   // IO19 - GT911 SDA  (TP_SDA)
#define PIN_TOUCH_SCL       45   // IO45 - GT911 SCL  (TP_SCL)

// ── I2C GPIO Expander konfiguracija ──────────────────────────────────────────

// ★ AKTIVNI EKSPANDER: Promijeniti ovu definiciju za prelazak na PCA9554
#define EXPANDER_TYPE_PCF8574   1   // Testna faza — NIJE glitch-free
#define EXPANDER_TYPE_PCA9554   2   // Finalna faza — glitch-free

#ifndef EXPANDER_TYPE
#define EXPANDER_TYPE           EXPANDER_TYPE_PCF8574  // ← PROMIJENITI OVDJE
#endif

#if EXPANDER_TYPE == EXPANDER_TYPE_PCF8574
#define EXPANDER_I2C_ADDR       0x38  // PCF8574AN, A0=A1=A2=GND
#define EXPANDER_HAS_OUTPUT_REG false // PCF8574 nema odvojeni output register
#else
#define EXPANDER_I2C_ADDR       0x20  // PCA9554, A0=A1=A2=GND
#define EXPANDER_HAS_OUTPUT_REG true  // PCA9554 ima odvojeni output register
#endif

// Expander pin mapiranje — isto za oba tipa expandera
#define EXP_PIN_RELAY1          0    // P0 → Relay 1 (Heating/Cooling)
#define EXP_PIN_RELAY2          1    // P1 → Relay 2 (Fan Speed 2)
#define EXP_PIN_RELAY3          2    // P2 → Relay 3 (Fan Speed 3)
#define EXP_PIN_RESERVE_OUT     3    // P3 → Rezerva output
#define EXP_PIN_RESERVE_IN0     4    // P4 → Rezerva input
#define EXP_PIN_RESERVE_IN1     5    // P5 → Rezerva input
#define EXP_PIN_WINDOW_SENSOR   6    // P6 → Window Sensor (Active LOW)
#define EXP_PIN_RESERVE_IN2     7    // P7 → Rezerva input

// ── Public API — HAL funkcije ────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

// Board inicijalizacija (display, touch, I2C expander, backlight)
void board_hal_init(void);
void hal_lvgl_tick_task(void *pv);
void hal_backlight_set(uint16_t level);  // 0-1023 input (auto-maps to 0-255)

// ── I2C Expander Adapter API ─────────────────────────────────────────────────
// Ove funkcije apstrahuju hardware — rade identično za PCF8574AN i PCA9554ADH.
// hvac.cpp i modbus_handler.cpp koriste SAMO ovaj API, nikad direktno I2C.

// Relej kontrola: relay_id = 1, 2, 3
void hal_relay_set(uint8_t relay_id, bool on);
bool hal_relay_is_on(uint8_t relay_id);
void hal_relay_all_off(void);
void hal_relay_sync(void); // Forsira pisanje trenutnog stanja na expander

// Window senzor: true = prozor otvoren (Active LOW na expander P6)
bool hal_window_is_open(void);

// Direktno čitanje svih expander input pinova (raw byte)
uint8_t hal_expander_read_inputs(void);

#ifdef __cplusplus
}
#endif
