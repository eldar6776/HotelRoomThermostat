#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

void wifi_manager_init();
void wifi_manager_set_ap(bool enable);
void wifi_manager_tick();
bool wifi_manager_has_credentials();

#endif // WIFI_MANAGER_H