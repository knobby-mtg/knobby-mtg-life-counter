#ifndef _WIRELESS_MANAGER_H
#define _WIRELESS_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "storage.h"

/* ---------- modes ----------
 * Cycle order is fixed: Disabled -> WiFi -> (future ESP-NOW -> Bluetooth -> WiFi AP) -> Disabled.
 * Phase 1 implements DISABLED and WIFI only; wireless_mode_supported() gates the cycle.
 */
typedef enum {
    WIRELESS_MODE_DISABLED = 0,
    WIRELESS_MODE_WIFI,
    WIRELESS_MODE_ESPNOW,
    WIRELESS_MODE_BLUETOOTH,
    WIRELESS_MODE_WIFI_AP,
    WIRELESS_MODE_COUNT,
} wireless_mode_t;

typedef enum {
    WIRELESS_STATE_INACTIVE,    /* Disabled, or just entered a mode */
    WIRELESS_STATE_IDLE,        /* Mode active, no connection attempt */
    WIRELESS_STATE_CONNECTING,
    WIRELESS_STATE_CONNECTED,
    WIRELESS_STATE_FAILED,
} wireless_state_t;

/* ---------- public API ---------- */

void             wireless_manager_init(void);     /* idempotent; safe to call lazily on first use */
void             wireless_poll(void);             /* force an immediate state-machine tick */

wireless_mode_t  wireless_current_mode(void);
wireless_state_t wireless_current_state(void);
bool             wireless_mode_supported(wireless_mode_t mode);
const char      *wireless_mode_label(wireless_mode_t mode);
void             wireless_set_mode(wireless_mode_t mode);
void             wireless_cycle_mode(void);       /* advances to next supported mode */

/* WiFi-mode-specific (meaningful only when current_mode() == WIFI) */
void             wifi_connect(const char *ssid, const char *password);
void             wifi_forget(const char *ssid);
void             wifi_current_ssid(char *out, size_t n);
int8_t           wifi_rssi(void);
uint32_t         wifi_ip(void);

/* Scan */
void             wifi_scan_start(void);
bool             wifi_scan_in_progress(void);
int              wifi_scan_result_count(void);    /* 0 if not done or empty */
void             wifi_scan_result(int idx, char *ssid_out, size_t n, int8_t *rssi_out, bool *is_secured_out);

/* Known networks (MRU) */
int                 wifi_known_count(void);       /* number of saved (non-empty) networks */
const wifi_net_t   *wifi_known(int idx);          /* idx 0 = most recently used */

/* ---------- backend hooks ----------
 * Implemented by knobby/wireless_backend.cpp on firmware (Arduino WiFi)
 * and by sim/sim_stubs.c for the screenshot simulator.
 */
void     wireless_hw_wifi_on(void);
void     wireless_hw_wifi_off(void);
void     wireless_hw_wifi_begin(const char *ssid, const char *password);
void     wireless_hw_wifi_disconnect(void);
int      wireless_hw_wifi_status(void);   /* 0=disconnected, 1=connecting, 2=connected, 3=failed */
int8_t   wireless_hw_wifi_rssi(void);
uint32_t wireless_hw_wifi_ip(void);
void     wireless_hw_wifi_current_ssid(char *out, size_t n);

void     wireless_hw_wifi_scan_start(void);
int      wireless_hw_wifi_scan_check(void);   /* -1 = running, else result count (>=0) */
void     wireless_hw_wifi_scan_result(int idx, char *ssid_out, size_t n, int8_t *rssi_out, bool *secured_out);
void     wireless_hw_wifi_scan_clear(void);

/* Backend status codes returned by wireless_hw_wifi_status() */
#define WIRELESS_HW_WIFI_DISCONNECTED 0
#define WIRELESS_HW_WIFI_CONNECTING   1
#define WIRELESS_HW_WIFI_CONNECTED    2
#define WIRELESS_HW_WIFI_FAILED       3

#ifdef __cplusplus
}
#endif

#endif /* _WIRELESS_MANAGER_H */
