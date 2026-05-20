#include "wifi_manager.h"
#include "debug_logger.h"
#include "settings.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Update.h>
#include <lvgl.h>

extern lv_obj_t *ui_SwitchStartAp;

static WiFiManager wm;
static bool s_portal_running = false;

// Custom HTML for OTA update, integrated into WiFiManager
const char* ota_html =
"<form method='POST' action='/update' enctype='multipart/form-data' style='font-family:sans-serif; text-align:center; margin-top:20px;'>"
"<h3>Thermostat OTA Update</h3>"
"<input type='file' name='update' accept='.bin' style='margin: 10px;'><br>"
"<input type='submit' value='Pokreni Update' style='padding: 10px 20px; font-size: 16px;'>"
"</form><hr>";

// Web server callback to safely register /update route once WebServer is created
static void bindServerCallback() {
    if (!wm.server) return;
    
    wm.server->on("/update", HTTP_POST, []() {
        wm.server->sendHeader("Connection", "close");
        wm.server->send(200, "text/plain", (Update.hasError()) ? "GRESKA!" : "OK! Restart...");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = wm.server->upload();
        if (upload.status == UPLOAD_FILE_START) {
            LOG_INFO("OTA Update Počeo: %s", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                LOG_INFO("OTA Update Završen: %u bajtova", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}

void wifi_manager_init() {
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(180); // 3 minutes
    wm.setCustomHeadElement(ota_html);
    wm.setWebServerCallback(bindServerCallback);
}

static void wifi_manager_stop_portal() {
    if (!s_portal_running) return;
    
    wm.stopConfigPortal();
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
        
        // Check if portal has timed out
        if (wm.getWebPortalActive() == false && s_portal_running) {
             LOG_INFO("[WiFi] AP Timeout from WiFiManager. Shutting down.");
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
        LOG_INFO("[WiFi] AP started: HotelThermostat (Web Server on port 80)");
    } else if (!enable && s_portal_running) {
        wifi_manager_stop_portal();
    }
}

bool wifi_manager_has_credentials() {
    // ESP32 automatically stores WiFi credentials in NVS. 
    // If WiFi.SSID() has content, we have saved credentials.
    return WiFi.SSID().length() > 0;
}
