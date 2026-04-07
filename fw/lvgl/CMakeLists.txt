SET(SOURCES ui_Main.c
    ui_Thermostat.c
    ui_Clean.c
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
    ui_img_arrow_forward_ios_48_png.c
    ui_img_fan_mode_png.c
    ui_img_arrow_back_ios_48_png.c
    ui_img_heating_png.c
    ui_img_cooling_png.c
    ui_img_do_not_touch_48_png.c
    fonts/ui_font_fontinterlight80.c)

add_library(ui ${SOURCES})
