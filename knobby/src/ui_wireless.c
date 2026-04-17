#include "ui_wireless.h"
#include "wireless_manager.h"
#include "settings.h"
#include "types.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

/* ---------- screens ---------- */
lv_obj_t *screen_wireless_menu    = NULL;
lv_obj_t *screen_wifi_networks    = NULL;
lv_obj_t *screen_wifi_scan        = NULL;
lv_obj_t *screen_wifi_password    = NULL;
lv_obj_t *screen_wireless_status  = NULL;

/* ---------- networks screen widgets ---------- */
static lv_obj_t *networks_list_container = NULL;
static lv_obj_t *btn_networks_scan       = NULL;
static lv_obj_t *label_networks_empty    = NULL;

/* ---------- scan screen widgets ---------- */
static lv_obj_t *scan_list_container = NULL;
static lv_obj_t *label_scan_hint     = NULL;
static lv_timer_t *scan_poll_timer   = NULL;

/* ---------- password screen widgets ---------- */
static lv_obj_t *label_password_ssid = NULL;
static lv_obj_t *textarea_password   = NULL;
static lv_obj_t *keyboard_password   = NULL;
static lv_obj_t *btn_password_save   = NULL;
static char pending_ssid[WIFI_SSID_LEN] = {0};

/* ---------- status screen widgets ---------- */
static lv_obj_t *label_status_mode   = NULL;
static lv_obj_t *label_status_state  = NULL;
static lv_obj_t *label_status_ssid   = NULL;
static lv_obj_t *label_status_ip     = NULL;
static lv_obj_t *label_status_rssi   = NULL;
static lv_timer_t *status_refresh_timer = NULL;

/* ---------- forward decls ---------- */
static void rebuild_wireless_menu(void);

/* ---------- helpers ---------- */
static const char *state_label(wireless_state_t s)
{
    switch (s) {
        case WIRELESS_STATE_INACTIVE:   return "Inactive";
        case WIRELESS_STATE_IDLE:       return "Idle";
        case WIRELESS_STATE_CONNECTING: return "Connecting...";
        case WIRELESS_STATE_CONNECTED:  return "Connected";
        case WIRELESS_STATE_FAILED:     return "Failed";
        default:                         return "?";
    }
}

static void format_ip(uint32_t ip, char *out, size_t n)
{
    /* Arduino IPAddress returns little-endian byte order */
    snprintf(out, n, "%u.%u.%u.%u",
             (unsigned)(ip & 0xFF),
             (unsigned)((ip >> 8) & 0xFF),
             (unsigned)((ip >> 16) & 0xFF),
             (unsigned)((ip >> 24) & 0xFF));
}

static const char *rssi_bars(int8_t rssi)
{
    if (rssi == 0)       return "    ";
    if (rssi >= -55)     return "||||";
    if (rssi >= -65)     return "||| ";
    if (rssi >= -75)     return "||  ";
    return "|   ";
}

/* ---------- list-row factory (reused across networks + scan) ---------- */

static lv_obj_t *make_list_row(lv_obj_t *parent, const char *primary, const char *secondary,
                                lv_color_t color, lv_event_cb_t click_cb,
                                lv_event_cb_t long_cb, void *user_data)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 300, 42);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    if (click_cb)
        lv_obj_add_event_cb(row, click_cb, LV_EVENT_SHORT_CLICKED, user_data);
    if (long_cb)
        lv_obj_add_event_cb(row, long_cb, LV_EVENT_LONG_PRESSED, user_data);

    lv_obj_t *lbl_primary = lv_label_create(row);
    lv_label_set_text(lbl_primary, primary);
    lv_label_set_long_mode(lbl_primary, LV_LABEL_LONG_DOT);
    lv_obj_set_width(lbl_primary, 220);
    lv_obj_set_style_text_color(lbl_primary, color, 0);
    lv_obj_set_style_text_font(lbl_primary, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_primary, LV_ALIGN_LEFT_MID, 0, 0);

    if (secondary && secondary[0]) {
        lv_obj_t *lbl_sec = lv_label_create(row);
        lv_label_set_text(lbl_sec, secondary);
        lv_obj_set_style_text_color(lbl_sec, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(lbl_sec, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_sec, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    return row;
}

/* ============================================================
 *  WIRELESS MENU (quad)
 * ============================================================ */

static void event_wireless_mode_cycle(lv_event_t *e)
{
    (void)e;
    wireless_cycle_mode();
    rebuild_wireless_menu();
}

static void event_open_network_config(lv_event_t *e)
{
    (void)e;
    if (wireless_current_mode() == WIRELESS_MODE_WIFI)
        open_wifi_networks_screen();
}

static void event_open_game_config(lv_event_t *e)
{
    /* Phase 1 placeholder — always greyed out, so this shouldn't fire. */
    (void)e;
}

static void event_open_status(lv_event_t *e)
{
    (void)e;
    open_wireless_status_screen();
}

static void rebuild_wireless_menu(void)
{
    lv_obj_t *old = screen_wireless_menu;
    bool is_currently_active = (lv_scr_act() == old);

    wireless_poll();
    bool mode_active = wireless_current_mode() != WIRELESS_MODE_DISABLED;
    /* Game Config is always greyed in Phase 1 — no implementation yet. Phase 2
     * will enable it once connected (state == CONNECTED). */
    bool game_config_enabled = false;

    static char mode_buf[32];
    snprintf(mode_buf, sizeof(mode_buf), "Mode:\n%s",
             wireless_mode_label(wireless_current_mode()));

    quad_item_t items[4] = {
        {mode_buf,          event_wireless_mode_cycle, true,               LV_EVENT_CLICKED, NULL, NULL},
        {"Network\nConfig", event_open_network_config, mode_active,        LV_EVENT_CLICKED, NULL, NULL},
        {"Game\nConfig",    event_open_game_config,    game_config_enabled, LV_EVENT_CLICKED, NULL, NULL},
        {"Status",          event_open_status,         mode_active,        LV_EVENT_CLICKED, NULL, NULL},
    };

    build_quad_screen(&screen_wireless_menu, items);
    if (is_currently_active)
        lv_scr_load(screen_wireless_menu);
    if (old)
        lv_obj_del(old);
}

void open_wireless_menu_screen(void)
{
    rebuild_wireless_menu();
    load_screen_if_needed(screen_wireless_menu);
}

/* ============================================================
 *  WIFI NETWORKS (MRU list + Scan button)
 * ============================================================ */

static void event_known_row_click(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    const wifi_net_t *net = wifi_known(idx);
    if (!net) return;
    wifi_connect(net->ssid, net->password);
    open_wireless_status_screen();
}

static void event_known_row_long(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    const wifi_net_t *net = wifi_known(idx);
    if (!net) return;
    char ssid_copy[WIFI_SSID_LEN];
    snprintf(ssid_copy, sizeof(ssid_copy), "%s", net->ssid);
    wifi_forget(ssid_copy);
    /* refresh list */
    open_wifi_networks_screen();
}

static void event_networks_scan(lv_event_t *e)
{
    (void)e;
    wifi_scan_start();
    open_wifi_scan_screen();
}

static void refresh_wifi_networks_ui(void)
{
    if (networks_list_container == NULL) return;
    lv_obj_clean(networks_list_container);

    int count = wifi_known_count();
    if (count == 0) {
        lv_obj_add_flag(networks_list_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_networks_empty, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(networks_list_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_networks_empty, LV_OBJ_FLAG_HIDDEN);

    char cur[WIFI_SSID_LEN];
    wifi_current_ssid(cur, sizeof(cur));

    for (int i = 0; i < count; i++) {
        const wifi_net_t *net = wifi_known(i);
        if (!net) continue;
        bool is_current = (strcmp(cur, net->ssid) == 0);
        make_list_row(networks_list_container, net->ssid,
                      is_current ? "connected" : "",
                      is_current ? lv_palette_main(LV_PALETTE_GREEN) : lv_color_white(),
                      event_known_row_click, event_known_row_long,
                      (void *)(intptr_t)i);
    }
}

static void build_wifi_networks_screen(void)
{
    screen_wifi_networks = lv_obj_create(NULL);
    lv_obj_set_size(screen_wifi_networks, 360, 360);
    lv_obj_set_style_bg_color(screen_wifi_networks, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_wifi_networks, 0, 0);
    lv_obj_set_scrollbar_mode(screen_wifi_networks, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(screen_wifi_networks, 0, 0);
    lv_obj_clear_flag(screen_wifi_networks, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_wifi_networks);
    lv_label_set_text(title, "Saved Networks");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    networks_list_container = lv_obj_create(screen_wifi_networks);
    lv_obj_remove_style_all(networks_list_container);
    lv_obj_set_size(networks_list_container, 310, 220);
    lv_obj_align(networks_list_container, LV_ALIGN_TOP_MID, 0, 66);
    lv_obj_set_flex_flow(networks_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(networks_list_container, 4, 0);
    lv_obj_set_scrollbar_mode(networks_list_container, LV_SCROLLBAR_MODE_OFF);

    label_networks_empty = lv_label_create(screen_wifi_networks);
    lv_label_set_text(label_networks_empty, "No saved networks.\nTap Scan below.");
    lv_obj_set_style_text_color(label_networks_empty, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_networks_empty, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_networks_empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_networks_empty, LV_ALIGN_CENTER, 0, 0);

    btn_networks_scan = make_button(screen_wifi_networks, "Scan", 120, 42, event_networks_scan);
    lv_obj_align(btn_networks_scan, LV_ALIGN_BOTTOM_MID, 0, -44);
}

void open_wifi_networks_screen(void)
{
    refresh_wifi_networks_ui();
    load_screen_if_needed(screen_wifi_networks);
}

/* ============================================================
 *  WIFI SCAN (list all APs)
 * ============================================================ */

static void event_scan_row_click(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    char ssid[WIFI_SSID_LEN];
    int8_t rssi;
    bool secured;
    wifi_scan_result(idx, ssid, sizeof(ssid), &rssi, &secured);
    if (ssid[0] == '\0') return;

    /* Already known? Skip password screen and reconnect directly. */
    int known = wifi_known_count();
    for (int i = 0; i < known; i++) {
        const wifi_net_t *net = wifi_known(i);
        if (net && strcmp(net->ssid, ssid) == 0) {
            wifi_connect(net->ssid, net->password);
            open_wireless_status_screen();
            return;
        }
    }

    /* Open networks (unsecured) can connect with no password */
    if (!secured) {
        wifi_connect(ssid, "");
        open_wireless_status_screen();
        return;
    }

    open_wifi_password_screen(ssid);
}

static int compare_rssi_desc(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    int8_t ra, rb;
    char ssid[WIFI_SSID_LEN];
    bool sec;
    wifi_scan_result(ia, ssid, sizeof(ssid), &ra, &sec);
    wifi_scan_result(ib, ssid, sizeof(ssid), &rb, &sec);
    return (int)rb - (int)ra;
}

static void refresh_wifi_scan_ui(void)
{
    if (scan_list_container == NULL) return;
    lv_obj_clean(scan_list_container);

    if (wifi_scan_in_progress()) {
        lv_label_set_text(label_scan_hint, "Scanning...");
        lv_obj_clear_flag(label_scan_hint, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    int count = wifi_scan_result_count();
    if (count == 0) {
        lv_label_set_text(label_scan_hint, "No networks found.");
        lv_obj_clear_flag(label_scan_hint, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(label_scan_hint, LV_OBJ_FLAG_HIDDEN);

    /* Sort indices by RSSI descending */
    int order[64];
    if (count > (int)(sizeof(order) / sizeof(order[0])))
        count = (int)(sizeof(order) / sizeof(order[0]));
    for (int i = 0; i < count; i++) order[i] = i;
    /* Simple qsort by RSSI */
    for (int i = 1; i < count; i++) {
        int key = order[i];
        int8_t kr; char ss[WIFI_SSID_LEN]; bool sec;
        wifi_scan_result(key, ss, sizeof(ss), &kr, &sec);
        int j = i - 1;
        while (j >= 0) {
            int8_t jr;
            wifi_scan_result(order[j], ss, sizeof(ss), &jr, &sec);
            if (jr >= kr) break;
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    for (int i = 0; i < count; i++) {
        int idx = order[i];
        char ssid[WIFI_SSID_LEN];
        int8_t rssi;
        bool secured;
        wifi_scan_result(idx, ssid, sizeof(ssid), &rssi, &secured);
        if (ssid[0] == '\0') continue;

        char secondary[16];
        snprintf(secondary, sizeof(secondary), "%s%s",
                 rssi_bars(rssi), secured ? " *" : "  ");

        make_list_row(scan_list_container, ssid, secondary, lv_color_white(),
                      event_scan_row_click, NULL, (void *)(intptr_t)idx);
    }
}

static void build_wifi_scan_screen(void)
{
    screen_wifi_scan = lv_obj_create(NULL);
    lv_obj_set_size(screen_wifi_scan, 360, 360);
    lv_obj_set_style_bg_color(screen_wifi_scan, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_wifi_scan, 0, 0);
    lv_obj_set_scrollbar_mode(screen_wifi_scan, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(screen_wifi_scan, 0, 0);
    lv_obj_clear_flag(screen_wifi_scan, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_wifi_scan);
    lv_label_set_text(title, "Nearby Networks");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    scan_list_container = lv_obj_create(screen_wifi_scan);
    lv_obj_remove_style_all(scan_list_container);
    lv_obj_set_size(scan_list_container, 310, 260);
    lv_obj_align(scan_list_container, LV_ALIGN_TOP_MID, 0, 66);
    lv_obj_set_flex_flow(scan_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scan_list_container, 4, 0);
    lv_obj_set_scrollbar_mode(scan_list_container, LV_SCROLLBAR_MODE_OFF);

    label_scan_hint = lv_label_create(screen_wifi_scan);
    lv_label_set_text(label_scan_hint, "Scanning...");
    lv_obj_set_style_text_color(label_scan_hint, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_scan_hint, &lv_font_montserrat_16, 0);
    lv_obj_align(label_scan_hint, LV_ALIGN_CENTER, 0, 0);
}

static void scan_poll_cb(lv_timer_t *t)
{
    (void)t;
    if (lv_scr_act() != screen_wifi_scan) {
        /* User navigated away — pause the poll until they return. */
        if (scan_poll_timer) lv_timer_pause(scan_poll_timer);
        return;
    }
    refresh_wifi_scan_ui();
    /* Once the scan has results (or failed and returned 0), stop polling. */
    if (!wifi_scan_in_progress() && scan_poll_timer)
        lv_timer_pause(scan_poll_timer);
}

void open_wifi_scan_screen(void)
{
    refresh_wifi_scan_ui();
    if (scan_poll_timer == NULL)
        scan_poll_timer = lv_timer_create(scan_poll_cb, 500, NULL);
    else
        lv_timer_resume(scan_poll_timer);
    load_screen_if_needed(screen_wifi_scan);
}

/* ============================================================
 *  WIFI PASSWORD (textarea + keyboard)
 * ============================================================ */

static void event_password_save(lv_event_t *e)
{
    (void)e;
    if (textarea_password == NULL) return;
    const char *pw = lv_textarea_get_text(textarea_password);
    wifi_connect(pending_ssid, pw ? pw : "");
    /* Clear textarea so password doesn't linger in UI */
    lv_textarea_set_text(textarea_password, "");
    open_wireless_status_screen();
}

static void build_wifi_password_screen(void)
{
    screen_wifi_password = lv_obj_create(NULL);
    lv_obj_set_size(screen_wifi_password, 360, 360);
    lv_obj_set_style_bg_color(screen_wifi_password, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_wifi_password, 0, 0);
    lv_obj_set_scrollbar_mode(screen_wifi_password, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(screen_wifi_password, 0, 0);
    lv_obj_clear_flag(screen_wifi_password, LV_OBJ_FLAG_SCROLLABLE);

    label_password_ssid = lv_label_create(screen_wifi_password);
    lv_label_set_text(label_password_ssid, "");
    lv_label_set_long_mode(label_password_ssid, LV_LABEL_LONG_DOT);
    lv_obj_set_width(label_password_ssid, 300);
    lv_obj_set_style_text_color(label_password_ssid, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_password_ssid, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_password_ssid, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_password_ssid, LV_ALIGN_TOP_MID, 0, 36);

    textarea_password = lv_textarea_create(screen_wifi_password);
    lv_obj_set_size(textarea_password, 260, 44);
    lv_obj_align(textarea_password, LV_ALIGN_TOP_MID, 0, 64);
    lv_textarea_set_max_length(textarea_password, WIFI_PASS_LEN - 1);
    lv_textarea_set_one_line(textarea_password, true);
    lv_textarea_set_password_mode(textarea_password, true);
    lv_textarea_set_placeholder_text(textarea_password, "password");

    btn_password_save = make_button(screen_wifi_password, "connect", 110, 40, event_password_save);
    lv_obj_align(btn_password_save, LV_ALIGN_TOP_MID, 0, 118);

    keyboard_password = lv_keyboard_create(screen_wifi_password);
    lv_obj_set_size(keyboard_password, 360, 180);
    lv_obj_align(keyboard_password, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard_password, textarea_password);
}

void open_wifi_password_screen(const char *ssid)
{
    if (ssid == NULL || ssid[0] == '\0') return;
    snprintf(pending_ssid, sizeof(pending_ssid), "%s", ssid);

    char title[64];
    snprintf(title, sizeof(title), "Password for\n%s", ssid);
    lv_label_set_text(label_password_ssid, title);
    lv_textarea_set_text(textarea_password, "");
    lv_keyboard_set_textarea(keyboard_password, textarea_password);

    load_screen_if_needed(screen_wifi_password);
}

/* ============================================================
 *  WIRELESS STATUS
 * ============================================================ */

static void status_timer_cb(lv_timer_t *t)
{
    (void)t;
    refresh_wireless_status_ui();
}

void refresh_wireless_status_ui(void)
{
    if (label_status_mode == NULL) return;

    wireless_poll();
    wireless_mode_t mode = wireless_current_mode();
    wireless_state_t state = wireless_current_state();

    char buf[64];

    snprintf(buf, sizeof(buf), "Mode: %s", wireless_mode_label(mode));
    lv_label_set_text(label_status_mode, buf);

    snprintf(buf, sizeof(buf), "State: %s", state_label(state));
    lv_label_set_text(label_status_state, buf);

    if (mode == WIRELESS_MODE_WIFI && state == WIRELESS_STATE_CONNECTED) {
        char ssid[WIFI_SSID_LEN];
        wifi_current_ssid(ssid, sizeof(ssid));
        snprintf(buf, sizeof(buf), "SSID: %s", ssid[0] ? ssid : "-");
        lv_label_set_text(label_status_ssid, buf);

        char ipstr[32];
        format_ip(wifi_ip(), ipstr, sizeof(ipstr));
        snprintf(buf, sizeof(buf), "IP: %s", ipstr);
        lv_label_set_text(label_status_ip, buf);

        snprintf(buf, sizeof(buf), "Signal: %s  (%d dBm)",
                 rssi_bars(wifi_rssi()), (int)wifi_rssi());
        lv_label_set_text(label_status_rssi, buf);
    } else {
        lv_label_set_text(label_status_ssid, "");
        lv_label_set_text(label_status_ip, "");
        lv_label_set_text(label_status_rssi, "");
    }
}

static void build_wireless_status_screen(void)
{
    screen_wireless_status = lv_obj_create(NULL);
    lv_obj_set_size(screen_wireless_status, 360, 360);
    lv_obj_set_style_bg_color(screen_wireless_status, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_wireless_status, 0, 0);
    lv_obj_set_scrollbar_mode(screen_wireless_status, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(screen_wireless_status, 0, 0);
    lv_obj_clear_flag(screen_wireless_status, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_wireless_status);
    lv_label_set_text(title, "Status");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    label_status_mode = lv_label_create(screen_wireless_status);
    lv_obj_set_style_text_color(label_status_mode, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status_mode, &lv_font_montserrat_16, 0);
    lv_obj_align(label_status_mode, LV_ALIGN_TOP_MID, 0, 84);

    label_status_state = lv_label_create(screen_wireless_status);
    lv_obj_set_style_text_color(label_status_state, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status_state, &lv_font_montserrat_16, 0);
    lv_obj_align(label_status_state, LV_ALIGN_TOP_MID, 0, 114);

    label_status_ssid = lv_label_create(screen_wireless_status);
    lv_label_set_long_mode(label_status_ssid, LV_LABEL_LONG_DOT);
    lv_obj_set_width(label_status_ssid, 300);
    lv_obj_set_style_text_color(label_status_ssid, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status_ssid, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_status_ssid, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_status_ssid, LV_ALIGN_TOP_MID, 0, 150);

    label_status_ip = lv_label_create(screen_wireless_status);
    lv_obj_set_style_text_color(label_status_ip, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status_ip, &lv_font_montserrat_16, 0);
    lv_obj_align(label_status_ip, LV_ALIGN_TOP_MID, 0, 180);

    label_status_rssi = lv_label_create(screen_wireless_status);
    lv_obj_set_style_text_color(label_status_rssi, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status_rssi, &lv_font_montserrat_16, 0);
    lv_obj_align(label_status_rssi, LV_ALIGN_TOP_MID, 0, 210);
}

void open_wireless_status_screen(void)
{
    refresh_wireless_status_ui();
    if (status_refresh_timer == NULL)
        status_refresh_timer = lv_timer_create(status_timer_cb, 500, NULL);
    else
        lv_timer_resume(status_refresh_timer);
    load_screen_if_needed(screen_wireless_status);
}

/* ============================================================
 *  Build-all entry point
 * ============================================================ */

void build_wireless_screens(void)
{
    build_wifi_networks_screen();
    build_wifi_scan_screen();
    build_wifi_password_screen();
    build_wireless_status_screen();
    /* wireless_menu is built on-demand (rebuild_wireless_menu) so it reflects current state. */
}
