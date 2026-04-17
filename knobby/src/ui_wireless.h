#ifndef _UI_WIRELESS_H
#define _UI_WIRELESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

extern lv_obj_t *screen_wireless_menu;
extern lv_obj_t *screen_wifi_networks;
extern lv_obj_t *screen_wifi_scan;
extern lv_obj_t *screen_wifi_password;
extern lv_obj_t *screen_wireless_status;

void build_wireless_screens(void);

void open_wireless_menu_screen(void);
void open_wifi_networks_screen(void);
void open_wifi_scan_screen(void);
void open_wifi_password_screen(const char *ssid);
void open_wireless_status_screen(void);

void refresh_wireless_status_ui(void);

/* Mode-change commit/cancel:
 *   commit — apply the staged mode to the radio. Called from sub-screen
 *            openers and from the dwell timer after the user stops tapping.
 *   cancel — discard the staged mode. Called from back-nav so the user
 *            can escape without applying an in-progress rotation.
 */
void commit_pending_wireless_mode(void);
void cancel_pending_wireless_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* _UI_WIRELESS_H */
