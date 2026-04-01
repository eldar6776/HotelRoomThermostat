SET(SOURCES ui_Main.c
    ui_PinEntry.c
    ui_Settings1.c
    ui_Settings2.c
    ui_Settings3.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_dnd_png.c
    ui_img_mur_png.c
    ui_img_hvac_png.c
    ui_img_thermo_inc_png.c
    ui_img_fan_mode_png.c
    ui_img_thermo_dec_png.c
    ui_img_heating_png.c
    ui_img_cooling_png.c
    ui_img_partly_cloudy_day_128_png.c
    ui_img_clear_night_png.c
    ui_img_cloudy_png.c
    ui_img_foggy_png.c
    ui_img_partly_cloudy_day_png.c
    ui_img_partly_cloudy_night_png.c
    ui_img_rainy_128_png.c
    ui_img_partly_cloudy_night_128_png.c
    ui_img_storm_128_png.c
    ui_img_sunny_128_png.c
    ui_img_cloudy_128_png.c
    ui_img_snowy_128_png.c
    ui_img_rainy_png.c
    ui_img_snowy_png.c
    ui_img_storm_png.c
    ui_img_sunny_day_png.c
    ui_img_foggy_128_png.c
    fonts/ui_font_fontinterlight80.c)

add_library(ui ${SOURCES})
