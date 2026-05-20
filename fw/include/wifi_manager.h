#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void wifi_manager_init(void);
void wifi_manager_set_ap(bool enable);
void wifi_manager_tick(void);
bool wifi_manager_has_credentials(void);

#ifdef __cplusplus
}
#endif
