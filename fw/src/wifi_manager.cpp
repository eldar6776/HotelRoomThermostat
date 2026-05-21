#include "wifi_manager.h"
#include "debug_logger.h"
#include "settings.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Update.h>
#include <LittleFS.h>
#include <lvgl.h>

extern lv_obj_t *ui_SwitchStartAp;

static WiFiManager wm;
static bool s_portal_running = false;

void wifi_manager_init() {
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(180); // 3 minutes
    
    // Configure WiFiManager menu to include the built-in firmware Update option
    std::vector<const char *> menu = {"wifi", "info", "param", "close", "sep", "update"};
    wm.setMenu(menu);
}

// ── Logo Upload Handler ───────────────────────────────────────────────────────
// Registrira /upload_logo POST rutu na WiFiManager web serveru.
// Poziva se odmah nakon startConfigPortal, dok je wm.server() dostupan.
static void register_logo_upload_route() {
    if (!wm.server.get()) return;

    // HTML forma koja se injektira kao custom parametar na portal stranicu
    static WiFiManagerParameter logo_param(
        "<hr><h3 style='margin-bottom:8px;'>&#128247; Logo baner (480x100 px PNG)</h3>"
        "<form method='POST' action='/upload_logo' enctype='multipart/form-data'>"
        "<input type='file' name='logo_file' accept='.png'"
        "  style='width:100%;padding:6px;margin-bottom:8px;box-sizing:border-box;'><br>"
        "<button type='submit'"
        "  style='width:100%;padding:10px;background:#0078d7;color:#fff;"
        "         border:none;border-radius:5px;font-size:16px;cursor:pointer;'>"
        "Ucitaj PNG u Flash</button>"
        "</form>"
    );
    // addParameter se smije pozvati samo jednom (pointer mora biti stabilan)
    static bool s_param_added = false;
    if (!s_param_added) {
        wm.addParameter(&logo_param);
        s_param_added = true;
    }

    // POST handler — prima multipart stream i upisuje ga u LittleFS
    wm.server.get()->on("/upload_logo", HTTP_POST,
        /* completion callback */
        []() {
            wm.server.get()->send(200, "text/html; charset=utf-8",
                "<meta charset='utf-8'>"
                "<h3>Uspjesno ucitano! Uredjaj se restartuje...</h3>"
                "<p>Ponovo se povezite za 10 sekundi.</p>");
            delay(1500);
            ESP.restart();
        },
        /* upload callback — prima svaki chunk multipart podataka */
        []() {
            HTTPUpload &upload = wm.server.get()->upload();
            static File s_upload_file;

            if (upload.status == UPLOAD_FILE_START) {
                LOG_INFO("[UPLOAD] Start: %s", upload.filename.c_str());
                LittleFS.remove("/logo_480x100.png"); // obriši staru verziju
                s_upload_file = LittleFS.open("/logo_480x100.png", FILE_WRITE);
                if (!s_upload_file) {
                    LOG_ERROR("[UPLOAD] Greska: nije moguce otvoriti fajl za pisanje!");
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (s_upload_file) {
                    s_upload_file.write(upload.buf, upload.currentSize);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (s_upload_file) {
                    s_upload_file.close();
                    LOG_INFO("[UPLOAD] Zavrsen, velicina: %u bajta", upload.totalSize);
                } else {
                    LOG_ERROR("[UPLOAD] Greska: fajl nije bio otvoren!");
                }
            }
        }
    );
    LOG_INFO("[WiFi] /upload_logo ruta registrovana.");
}

static void wifi_manager_stop_portal() {
    if (!s_portal_running) return;
    
    // Only call stopConfigPortal if it is actually still active inside WiFiManager to avoid duplicate shutdown crashes
    if (wm.getConfigPortalActive()) {
        wm.stopConfigPortal();
    }
    
    WiFi.mode(WIFI_OFF);
    s_portal_running = false;
    g_wifi_ap_active = false;
    LOG_INFO("[WiFi] Config Portal stopped.");

    // Sync UI Switch if it exists
    if (ui_SwitchStartAp) {
        lv_obj_clear_state(ui_SwitchStartAp, LV_STATE_CHECKED);
    }
    
    // Apply DFS if the screen is currently in screensaver
    if (inactivity_is_screensaver_active()) {
        setCpuFrequencyMhz(80);
        LOG_INFO("[CFG] CPU clock reduced to 80MHz (Screensaver Active & WiFi Stopped)");
    }
}

void wifi_manager_tick() {
    if (s_portal_running) {
        wm.process(); // Handles the web server and timeout
        
        // Check if portal has timed out or closed
        if (wm.getConfigPortalActive() == false && s_portal_running) {
             LOG_INFO("[WiFi] AP Timeout or close from WiFiManager. Shutting down.");
             wifi_manager_stop_portal();
        }
    }
}

void wifi_manager_set_ap(bool enable) {
    if (enable && !s_portal_running) {
        LOG_INFO("[WiFi] Starting Config Portal...");
        setCpuFrequencyMhz(240);
        s_portal_running = true;
        g_wifi_ap_active = true; // Use existing global flag
        wm.setConfigPortalBlocking(false);
        wm.startConfigPortal("HotelThermostat", "12345678");
        register_logo_upload_route(); // Registruj upload rutu odmah nakon pokretanja
        LOG_INFO("[WiFi] AP started: HotelThermostat (Web Server on port 80)");
    } else if (!enable && s_portal_running) {
        wifi_manager_stop_portal();
    }
}

bool wifi_manager_has_credentials() {
    // ESP32 automatically stores WiFi credentials in NVS. 
    // If WiFi driver is off, WiFi.SSID() is empty.
    // We temporarily initialize STA mode to read the saved SSID from NVS.
    wifi_mode_t old_mode = WiFi.getMode();
    if (old_mode == WIFI_OFF || old_mode == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA);
    }
    
    bool has_creds = (WiFi.SSID().length() > 0) || (wm.getWiFiIsSaved());
    
    if (old_mode == WIFI_OFF || old_mode == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_OFF);
    }
    return has_creds;
}
