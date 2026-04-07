#include "wifi_manager.h"
#include "debug_logger.h"
#include <Arduino.h>
#include <WiFi.h>

void wifi_manager_set_ap(bool enable)
{
    if (enable) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("HotelThermostat", "12345678");
        LOG_INFO("[WiFi] AP started: %s",
                 WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        LOG_INFO("[WiFi] AP stopped");
    }
}
