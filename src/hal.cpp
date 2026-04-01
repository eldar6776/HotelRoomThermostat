#include "hal.h"
#include "debug_logger.h"
#include <Arduino_GFX_Library.h>
#include <ESP32_4848S040.h>
#include <Wire.h>
#include <Touch_GT911.h>
#include <lvgl.h>

// ── Display (ST7701S 480×480 RGB) ────────────────────────────────────────────
// Using the correct two-object approach from the verified working example:
//   1. Arduino_ESP32SPI  — drives the ST7701 SPI configuration bus
//   2. Arduino_ESP32RGBPanel — the RGB parallel data bus (with correct 1,1 polarity)
//   3. Arduino_RGB_Display — combines both and applies the device-specific init table
static Arduino_ESP32SPI *s_spi_bus = new Arduino_ESP32SPI(
    GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

static Arduino_ESP32RGBPanel *s_rgb_panel = new Arduino_ESP32RGBPanel(
    18 /* DE  */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */,
    11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */,  0 /* R4 */,
     8 /* G0 */, 20 /* G1 */,  3 /* G2 */, 46 /* G3 */,  9 /* G4 */, 10 /* G5 */,
     4 /* B0 */,  5 /* B1 */,  6 /* B2 */,  7 /* B3 */, 15 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

static Arduino_RGB_Display *s_gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, s_rgb_panel, 0 /* rotation */, true /* auto_flush */,
    s_spi_bus, GFX_NOT_DEFINED /* RST */,
    st7701_4848s040_init_operations, sizeof(st7701_4848s040_init_operations));

// ── Touch (GT911: SDA=IO19, SCL=IO45) ───────────────────────────────────────
static Touch_GT911 s_touch(PIN_TOUCH_SDA, PIN_TOUCH_SCL,
                           -1 /* INT – not connected */,
                           -1 /* RST – not connected */,
                           480, 480);

// ── LVGL draw buffer (single buffer in internal SRAM for speed) ──────────────
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t        *s_buf1 = nullptr;
#define DISP_BUF_LINES  80

// ── LVGL flush callback (v8 API) ─────────────────────────────────────────────
static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    s_gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    lv_disp_flush_ready(drv);
}

// ── LVGL touch read callback (v8 API) ────────────────────────────────────────
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    s_touch.read();
    if (s_touch.isTouched && s_touch.touches > 0) {
        data->point.x = 480 - s_touch.points[0].x;
        data->point.y = 480 - s_touch.points[0].y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ── Backlight (analogWrite, 0-255) ───────────────────────────────────────────
void hal_backlight_set(uint16_t level)
{
    // Map 0-1023 (PWM duty cycle range) → 0-255 (analogWrite range)
    uint8_t pwm = (level > 1023) ? 255 : (level * 255 / 1023);
    analogWrite(PIN_LCD_BL, pwm);
    LOG_DEBUG("Backlight: %u → PWM %u", level, pwm);
}

// ── LVGL tick task (stub — LV_TICK_CUSTOM=1 handles timing) ──────────────────
void hal_lvgl_tick_task(void *pv)
{
    (void)pv;
    vTaskDelete(nullptr);
}

// ── Main init ────────────────────────────────────────────────────────────────
void board_hal_init(void)
{
    // IO39 (RELAY3) and IO48 (RS485_RTS) double as ST7701 SPI init bus (CS=39,
    // SCK=48). Configure them AFTER s_gfx->begin() once SPI init is done.
    LOG_INFO("[HAL] Step 1: GPIO...");
    Serial.flush();
    pinMode(PIN_RELAY1, OUTPUT);         digitalWrite(PIN_RELAY1, LOW);
    pinMode(PIN_RELAY2, OUTPUT);         digitalWrite(PIN_RELAY2, LOW);
    pinMode(PIN_WINDOW_SENSOR, INPUT_PULLUP);

    LOG_INFO("[HAL] Step 2: Backlight (analogWrite)...");
    Serial.flush();
    pinMode(PIN_LCD_BL, OUTPUT);
    hal_backlight_set(128);  // ~50% until NVS value is loaded

    LOG_INFO("[HAL] Step 3: gfx->begin()...");
    Serial.flush();
    if (!s_gfx->begin()) {
        LOG_ERROR("[HAL] Display begin() FAILED");
    }
    LOG_INFO("[HAL] Step 3 done.");
    Serial.flush();

    // Now safe to configure the SPI-shared pins for relay/RS485
    pinMode(PIN_RELAY3, OUTPUT);         digitalWrite(PIN_RELAY3, LOW);
    pinMode(PIN_RS485_RTS, OUTPUT);      digitalWrite(PIN_RS485_RTS, LOW);

    LOG_INFO("[HAL] Step 4: lv_init()...");
    Serial.flush();
    lv_init();

    LOG_INFO("[HAL] Step 5: SRAM alloc...");
    Serial.flush();
    uint32_t buf_px = 480 * DISP_BUF_LINES;
    s_buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t),
                                             MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_buf1) {
        LOG_ERROR("[HAL] Cannot allocate LVGL draw buffer in SRAM!");
    }
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, NULL, buf_px);

    LOG_INFO("[HAL] Step 6: register LVGL display driver (v8)...");
    Serial.flush();
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = 480;
    disp_drv.ver_res  = 480;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&disp_drv);

    LOG_INFO("[HAL] Step 7: touch init (SDA=%d, SCL=%d)...", PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    Serial.flush();
    // Wire.begin() is called inside s_touch.begin() - no need to call twice!
    s_touch.begin();

    LOG_INFO("[HAL] Step 8: register LVGL input driver (v8)...");
    Serial.flush();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    LOG_INFO("[HAL] Init OK");
}
