#pragma once
#include <Arduino.h>

// ── Pin definitions (FSD §1) ─────────────────────────────────────────────────
#define PIN_RELAY1          40   // IO40 – Relay 1 (Heating/Cooling or Speed 1)
#define PIN_RELAY2          2    // IO2  – Relay 2 (Speed 2)
#define PIN_RELAY3          39   // IO39 – Relay 3 (Speed 3)
#define PIN_NTC             1    // IO1  – NTC 100k B3950 (ADC1_CH0)
#define PIN_WINDOW_SENSOR   41   // IO41 – Window sensor (Active LOW)
#define PIN_RS485_RTS       48   // IO48 – THVD1500 Transmit Enable
#define PIN_LCD_BL          38   // IO38 – LCD Backlight (PWM)
#define PIN_TOUCH_SDA       19   // IO19 – GT911 SDA  (TP_SDA)
#define PIN_TOUCH_SCL       45   // IO45 – GT911 SCL  (TP_SCL)

// ── Public API ───────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif
void board_hal_init(void);
void hal_lvgl_tick_task(void *pv);
void hal_backlight_set(uint16_t level);  // 0-1023 input (auto-maps to 0-255)
#ifdef __cplusplus
}
#endif
