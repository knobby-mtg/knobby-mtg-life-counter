#include "wireless_manager.h"
#include "storage.h"
#include <lvgl.h>
#include <string.h>

/* ---------- state ---------- */

static bool             initialized      = false;
static wireless_mode_t  current_mode     = WIRELESS_MODE_DISABLED;
static wireless_state_t current_state    = WIRELESS_STATE_INACTIVE;
static lv_timer_t      *poll_timer       = NULL;

/* MRU list in RAM; mirrors NVS-persisted cached_wifi_nets via storage API */
static wifi_net_t mru_nets[WIFI_NET_COUNT];

/* Pending-save: set when wifi_connect is called; committed to MRU on CONNECTED */
static bool pending_save   = false;
static wifi_net_t pending_net;

/* ---------- helpers ---------- */

static bool net_is_empty(const wifi_net_t *n)
{
    return n->ssid[0] == '\0';
}

static int mru_populated_count(void)
{
    int n = 0;
    for (int i = 0; i < WIFI_NET_COUNT; i++) {
        if (net_is_empty(&mru_nets[i])) break;
        n++;
    }
    return n;
}

static int mru_find(const char *ssid)
{
    for (int i = 0; i < WIFI_NET_COUNT; i++) {
        if (!net_is_empty(&mru_nets[i]) && strcmp(mru_nets[i].ssid, ssid) == 0)
            return i;
    }
    return -1;
}

/* Mirrors mru_use_name() in rename.c: promote (or insert) ssid+password to slot 0.
 * Persists to NVS.
 */
static void mru_promote(const wifi_net_t *net)
{
    int found = mru_find(net->ssid);
    wifi_net_t tmp;

    if (found >= 0) {
        tmp = mru_nets[found];
        memmove(&mru_nets[1], &mru_nets[0], (size_t)found * sizeof(wifi_net_t));
        mru_nets[0] = tmp;
        /* refresh password in case it changed */
        memcpy(mru_nets[0].password, net->password, sizeof(mru_nets[0].password));
    } else {
        int count = mru_populated_count();
        int shift = (count < WIFI_NET_COUNT) ? count : (WIFI_NET_COUNT - 1);
        if (shift > 0)
            memmove(&mru_nets[1], &mru_nets[0], (size_t)shift * sizeof(wifi_net_t));
        mru_nets[0] = *net;
    }

    nvs_set_wifi_nets(mru_nets);
}

static void mru_remove(const char *ssid)
{
    int found = mru_find(ssid);
    if (found < 0) return;

    int count = mru_populated_count();
    int remaining = count - found - 1;
    if (remaining > 0)
        memmove(&mru_nets[found], &mru_nets[found + 1], (size_t)remaining * sizeof(wifi_net_t));
    memset(&mru_nets[count - 1], 0, sizeof(wifi_net_t));

    nvs_set_wifi_nets(mru_nets);
}

/* ---------- state machine ---------- */

static void wireless_poll_internal(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return;

    int hw = wireless_hw_wifi_status();
    wireless_state_t next = current_state;

    switch (hw) {
        case WIRELESS_HW_WIFI_CONNECTED:
            next = WIRELESS_STATE_CONNECTED;
            if (pending_save) {
                mru_promote(&pending_net);
                pending_save = false;
                memset(&pending_net, 0, sizeof(pending_net));
            }
            break;
        case WIRELESS_HW_WIFI_CONNECTING:
            next = WIRELESS_STATE_CONNECTING;
            break;
        case WIRELESS_HW_WIFI_FAILED:
            next = WIRELESS_STATE_FAILED;
            /* Don't save failed creds: intentionally drop pending_save on failure */
            pending_save = false;
            memset(&pending_net, 0, sizeof(pending_net));
            break;
        case WIRELESS_HW_WIFI_DISCONNECTED:
        default:
            /* Only transition to IDLE from a prior active state; don't override FAILED */
            if (current_state == WIRELESS_STATE_CONNECTING)
                next = WIRELESS_STATE_FAILED;
            else if (current_state != WIRELESS_STATE_FAILED)
                next = WIRELESS_STATE_IDLE;
            break;
    }

    current_state = next;
}

void wireless_poll(void)
{
    wireless_poll_internal();
}

static void wireless_tick(lv_timer_t *t)
{
    (void)t;
    wireless_poll_internal();
}

static void ensure_poll_timer(void)
{
    if (poll_timer == NULL)
        poll_timer = lv_timer_create(wireless_tick, 500, NULL);
    else
        lv_timer_resume(poll_timer);
}

static void pause_poll_timer(void)
{
    if (poll_timer != NULL)
        lv_timer_pause(poll_timer);
}

/* ---------- init ---------- */

void wireless_manager_init(void)
{
    if (initialized) return;
    nvs_get_wifi_nets(mru_nets);
    current_mode = WIRELESS_MODE_DISABLED;
    current_state = WIRELESS_STATE_INACTIVE;
    initialized = true;
}

/* ---------- mode ---------- */

wireless_mode_t wireless_current_mode(void)
{
    return current_mode;
}

wireless_state_t wireless_current_state(void)
{
    return current_state;
}

bool wireless_mode_supported(wireless_mode_t mode)
{
    switch (mode) {
        case WIRELESS_MODE_DISABLED:
        case WIRELESS_MODE_WIFI:
            return true;
        default:
            return false;
    }
}

const char *wireless_mode_label(wireless_mode_t mode)
{
    switch (mode) {
        case WIRELESS_MODE_DISABLED:  return "Disabled";
        case WIRELESS_MODE_WIFI:      return "WiFi";
        case WIRELESS_MODE_ESPNOW:    return "ESP-NOW";
        case WIRELESS_MODE_BLUETOOTH: return "Bluetooth";
        case WIRELESS_MODE_WIFI_AP:   return "WiFi AP";
        default:                       return "?";
    }
}

void wireless_set_mode(wireless_mode_t mode)
{
    wireless_manager_init();

    if (!wireless_mode_supported(mode)) return;
    if (mode == current_mode) return;

    /* Tear down current mode */
    if (current_mode == WIRELESS_MODE_WIFI) {
        wireless_hw_wifi_disconnect();
        wireless_hw_wifi_off();
        pause_poll_timer();
    }

    current_mode = mode;
    current_state = WIRELESS_STATE_INACTIVE;
    pending_save = false;
    memset(&pending_net, 0, sizeof(pending_net));

    /* Bring up new mode */
    if (mode == WIRELESS_MODE_WIFI) {
        wireless_hw_wifi_on();
        ensure_poll_timer();
        current_state = WIRELESS_STATE_IDLE;

        /* Auto-connect to most-recent network, if any */
        if (!net_is_empty(&mru_nets[0])) {
            wireless_hw_wifi_begin(mru_nets[0].ssid, mru_nets[0].password);
            current_state = WIRELESS_STATE_CONNECTING;
            /* Already in MRU — no pending save needed */
        }
    }
}

void wireless_cycle_mode(void)
{
    wireless_manager_init();
    for (int i = 1; i < WIRELESS_MODE_COUNT; i++) {
        wireless_mode_t next = (wireless_mode_t)((current_mode + i) % WIRELESS_MODE_COUNT);
        if (wireless_mode_supported(next)) {
            wireless_set_mode(next);
            return;
        }
    }
}

/* ---------- WiFi connect / forget ---------- */

void wifi_connect(const char *ssid, const char *password)
{
    if (current_mode != WIRELESS_MODE_WIFI || ssid == NULL || ssid[0] == '\0') return;

    memset(&pending_net, 0, sizeof(pending_net));
    strncpy(pending_net.ssid, ssid, WIFI_SSID_LEN - 1);
    if (password)
        strncpy(pending_net.password, password, WIFI_PASS_LEN - 1);
    pending_save = true;

    wireless_hw_wifi_disconnect();
    wireless_hw_wifi_begin(ssid, password ? password : "");
    current_state = WIRELESS_STATE_CONNECTING;
}

void wifi_forget(const char *ssid)
{
    if (ssid == NULL || ssid[0] == '\0') return;

    /* If currently connected to this SSID, disconnect */
    char cur[WIFI_SSID_LEN];
    wireless_hw_wifi_current_ssid(cur, sizeof(cur));
    if (strcmp(cur, ssid) == 0) {
        wireless_hw_wifi_disconnect();
        current_state = WIRELESS_STATE_IDLE;
    }

    mru_remove(ssid);
}

void wifi_current_ssid(char *out, size_t n)
{
    if (current_mode != WIRELESS_MODE_WIFI || out == NULL || n == 0) {
        if (out && n > 0) out[0] = '\0';
        return;
    }
    wireless_hw_wifi_current_ssid(out, n);
}

int8_t wifi_rssi(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return 0;
    return wireless_hw_wifi_rssi();
}

uint32_t wifi_ip(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return 0;
    return wireless_hw_wifi_ip();
}

/* ---------- scan ---------- */

void wifi_scan_start(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return;
    wireless_hw_wifi_scan_start();
}

bool wifi_scan_in_progress(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return false;
    return wireless_hw_wifi_scan_check() < 0;
}

int wifi_scan_result_count(void)
{
    if (current_mode != WIRELESS_MODE_WIFI) return 0;
    int n = wireless_hw_wifi_scan_check();
    return (n < 0) ? 0 : n;
}

void wifi_scan_result(int idx, char *ssid_out, size_t n, int8_t *rssi_out, bool *is_secured_out)
{
    if (ssid_out && n > 0) ssid_out[0] = '\0';
    if (rssi_out) *rssi_out = 0;
    if (is_secured_out) *is_secured_out = false;
    if (current_mode != WIRELESS_MODE_WIFI) return;
    wireless_hw_wifi_scan_result(idx, ssid_out, n, rssi_out, is_secured_out);
}

/* ---------- known networks ---------- */

int wifi_known_count(void)
{
    return mru_populated_count();
}

const wifi_net_t *wifi_known(int idx)
{
    if (idx < 0 || idx >= WIFI_NET_COUNT) return NULL;
    if (net_is_empty(&mru_nets[idx])) return NULL;
    return &mru_nets[idx];
}
