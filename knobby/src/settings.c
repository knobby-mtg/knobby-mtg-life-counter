#include "settings.h"
#include "hw.h"
#include "storage.h"
#include <string.h>
#include "dice.h"
#include "timer.h"
#include "game_mode.h"
#include "damage_log.h"
#include "rename.h"
#include "game.h"
#include "mana.h"
#include "net_sync.h"
#include "ui_1p.h"
#include "ui_mp.h"
#include "ui_player_menu.h"

// Forward declarations for cross-module calls
extern void reset_all_values(void);
extern void back_to_main(void);

// ---------- screens ----------
lv_obj_t *screen_quad_menu = NULL;
lv_obj_t *screen_tools_menu = NULL;
lv_obj_t *screen_settings = NULL;
lv_obj_t *screen_battery = NULL;
lv_obj_t *screen_rotate = NULL;

// ---------- widgets ----------
static lv_obj_t *arc_brightness = NULL;
static lv_obj_t *label_settings_value = NULL;
static lv_obj_t *label_settings_hint = NULL;
static lv_obj_t *label_settings_battery = NULL;
static lv_obj_t *label_settings_battery_detail = NULL;
static lv_obj_t *label_rotate_value = NULL;

// ---------- quadrant menu builder ----------
void build_quad_screen(lv_obj_t **screen, quad_item_t items[4])
{
    int i;
    static const lv_coord_t qx[4] = {0,   182, 0,   182};
    static const lv_coord_t qy[4] = {0,   0,   182, 182};
    static const lv_coord_t lx[4] = {10, -10, 10, -10};
    static const lv_coord_t ly[4] = {15,  15, -15, -15};

    *screen = lv_obj_create(NULL);
    lv_obj_set_size(*screen, 360, 360);
    lv_obj_set_style_bg_color(*screen, lv_color_black(), 0);
    lv_obj_set_style_border_width(*screen, 0, 0);
    lv_obj_set_scrollbar_mode(*screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(*screen, 0, 0);

    for (i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(*screen);
        lv_obj_remove_style_all(btn);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_set_size(btn, 178, 178);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_pos(btn, qx[i], qy[i]);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        if (items[i].cb != NULL && items[i].enabled) {
            lv_obj_add_event_cb(btn, items[i].cb, items[i].event, items[i].user_data);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x111111), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);
            lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        }

        if (items[i].icon != NULL && items[i].icon_font != NULL) {
            lv_obj_t *icon_lbl = lv_label_create(btn);
            lv_label_set_text(icon_lbl, items[i].icon);
            lv_obj_set_style_text_font(icon_lbl, items[i].icon_font, 0);
            lv_obj_set_style_text_color(icon_lbl,
                items[i].enabled ? lv_color_white() : lv_color_hex(0x555555), 0);
            lv_obj_align(icon_lbl, LV_ALIGN_CENTER, lx[i], ly[i] - 18);
        }

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i].label);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, lx[i],
            (items[i].icon != NULL) ? ly[i] + 10 : ly[i]);

        if (!items[i].enabled) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x555555), 0);
        }
    }
}

// ---------- refresh ----------
static void refresh_brightness_ring(void)
{
    lv_arc_set_value(arc_brightness, brightness_percent);

    lv_obj_set_style_arc_color(arc_brightness, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_brightness, 18, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc_brightness, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_brightness, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_brightness, true, LV_PART_INDICATOR);
}

void refresh_settings_ui(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", brightness_percent);
    lv_label_set_text(label_settings_value, buf);
    refresh_brightness_ring();
}

void refresh_battery_ui(void)
{
    char buf[32];
    char detail_buf[48];

    battery_percent = read_battery_percent();
    if (label_settings_battery == NULL) return;

    if (battery_percent < 0) {
        lv_label_set_text(label_settings_battery, "Battery: --%");
        if (label_settings_battery_detail != NULL) {
            lv_label_set_text(label_settings_battery_detail, "No calibrated reading");
        }
        return;
    }

    snprintf(buf, sizeof(buf), "Battery: %d%%", battery_percent);
    lv_label_set_text(label_settings_battery, buf);
    if (label_settings_battery_detail != NULL) {
        snprintf(detail_buf, sizeof(detail_buf), "%.2fV calibrated", battery_voltage);
        lv_label_set_text(label_settings_battery_detail, detail_buf);
    }
}

void refresh_rotate_ui(void)
{
    char buf[16];
    if (label_rotate_value == NULL) return;
    snprintf(buf, sizeof(buf), "%d°", nvs_get_display_rotation() * 90);
    lv_label_set_text(label_rotate_value, buf);
}

// ---------- navigation ----------
void open_quad_menu(void)
{
    load_screen_if_needed(screen_quad_menu);
}

void open_settings_screen(void)
{
    // No forced battery read here – the brightness settings page does not display
    // battery info. The battery screen entry point forces its own sample.
    refresh_settings_ui();
    load_screen_if_needed(screen_settings);
}

// ---------- events ----------
// ---------- toggle colors ----------
static const uint32_t TOGGLE_ON  = 0x1B5E20;  /* dark green */
static const uint32_t TOGGLE_OFF = 0x4A1010;  /* dark red */

static void set_btn_color(lv_obj_t *btn, uint32_t color)
{
    if (btn != NULL) lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
}

static uint32_t autodim_color(int index)
{
    static const uint32_t colors[AUTO_DIM_COUNT] = {
        0x37474F, 0x1B5E20, 0x0D47A1, 0x4A148C  /* grey, green, blue, purple */
    };
    return (index >= 0 && index < AUTO_DIM_COUNT) ? colors[index] : 0x37474F;
}
static uint32_t orientation_color(int mode)
{
    switch (mode) {
        case ORIENTATION_MODE_CENTRIC: return TOGGLE_ON;
        case ORIENTATION_MODE_TABLETOP:  return 0x0D47A1;
        default:                   return TOGGLE_OFF;
    }
}

static uint32_t color_mode_color(int mode)
{
    return (mode == COLOR_MODE_LIFE) ? 0x4A148C : 0x0D47A1; /* purple / blue */
}

static uint32_t deselect_color(int index)
{
    static const uint32_t colors[DESELECT_COUNT] = {
        0x37474F, 0x1B5E20, 0x0D47A1, 0x4A148C  /* grey, green, blue, purple */
    };
    return (index >= 0 && index < DESELECT_COUNT) ? colors[index] : 0x1A1A2E;
}

static void event_quad_screen_settings(lv_event_t *e)
{
    (void)e;
    lv_scr_load(settings_pages[0]);
}

static const char *autodim_label(int index)
{
    switch (index) {
        case AUTO_DIM_15S: return "Auto-dim\n15s";
        case AUTO_DIM_30S: return "Auto-dim\n30s";
        case AUTO_DIM_60S: return "Auto-dim\n60s";
        default:           return "Auto-dim\nOFF";
    }
}

static const char *color_mode_label(int mode)
{
    switch (mode) {
        case COLOR_MODE_LIFE:   return "Colors\nLife";
        default:                return "Colors\nPlayer";
    }
}

static const char *deselect_label(int index)
{
    switch (index) {
        case DESELECT_5S:    return "Deselect\n5s";
        case DESELECT_15S:   return "Deselect\n15s";
        case DESELECT_30S:   return "Deselect\n30s";
        default:             return "Deselect\nNever";
    }
}

static const char *orientation_mode_label(int mode)
{
    switch (mode) {
        case ORIENTATION_MODE_CENTRIC: return "Orientation\nCentric";
        case ORIENTATION_MODE_TABLETOP:  return "Orientation\nTabletop";
        default:                   return "Orientation\nAbsolute";
    }
}

static const char *auto_eliminate_label(int val)
{
    return val ? "Auto\nElimination\nON" : "Auto\nElimination\nOFF";
}

// ---------- declarative settings ----------
/* Every user setting lives in this one table. Pages, "More" chaining,
   back-navigation, and sim navigation are all derived from it: to add,
   remove, or reorder a setting, edit only this table (plus its label,
   color, and NVS functions). Label fns must return strings that
   lv_label_set_text may copy — never switch the refresh to
   lv_label_set_text_static. */

static int autodim_get(void) { return nvs_get_auto_dim(); }
static void autodim_set(int v)
{
    nvs_set_auto_dim(v);
    if (v == AUTO_DIM_OFF && dimmed) {
        dimmed = false;
        brightness_apply();
    }
}

static uint32_t toggle_color(int val)
{
    return val ? TOGGLE_ON : TOGGLE_OFF;
}

void open_battery_screen(void)
{
    update_battery_measurement(true);
    refresh_battery_ui();
    lv_scr_load(screen_battery);
}

void open_rotate_screen(void)
{
    refresh_rotate_ui();
    lv_scr_load(screen_rotate);
}

void change_display_rotation(int dir)
{
    int v = (nvs_get_display_rotation() +
             (dir > 0 ? 1 : DISPLAY_ROTATION_COUNT - 1)) % DISPLAY_ROTATION_COUNT;
    nvs_set_display_rotation(v);
    refresh_rotate_ui();
    menu_facing_refresh();
}

/* Player-scoped menu screens that should face the acting player when
   menu facing is enabled. screen_select/screen_damage are the commander
   damage picker and editor: player-scoped in multiplayer (opened from the
   player menu, which sets cmd_damage_target), and a no-op in 1-player
   since mp_player_seat_rotation() returns 0 there. */
static bool screen_is_player_menu(lv_obj_t *screen)
{
    return screen == screen_player_menu ||
           screen == screen_eliminated_player_menu ||
           screen == screen_player_all_damage ||
           screen == screen_counter_menu ||
           screen == screen_counter_edit ||
           screen == screen_player_color_menu ||
           screen == screen_player_color_picker ||
           screen == screen_player_name ||
           screen == screen_select ||
           screen == screen_damage;
}

/* Single applier for the effective display rotation: the user's physical
   rotation, plus the acting player's seat on player menus when the
   "Menus: Face Player" toggle is on. Idempotent — recomputes from the
   active screen, so any navigation path can call it safely. */
void menu_facing_refresh(void)
{
    static int applied = -1;
    int target = nvs_get_display_rotation();

    if (nvs_get_menu_facing() && screen_is_player_menu(lv_scr_act()))
        target = (target + mp_player_seat_rotation(menu_player)) & 3;
    if (target == applied) return;
    applied = target;
    display_apply_rotation(target);
    /* Repaint the whole frame immediately: the panel's GRAM still holds the
       old orientation the instant the MADCTL flags change. */
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
}

static const char *random_first_label(int val)
{
    return val ? "Random\nFirst\nON" : "Random\nFirst\nOFF";
}

static const char *menu_facing_label(int val)
{
    return val ? "Menus\nFace\nPlayer" : "Menus\nFixed";
}

static const char *multi_select_label(int val)
{
    return val ? "Multi-\nSelect\nON" : "Multi-\nSelect\nOFF";
}

static void multi_select_set(int v)
{
    nvs_set_multi_select(v);
    if (v == 0) {
        /* Turning multi-select off: drop any lingering multi-selection so the
           single-select rules apply cleanly on return to the life screen. */
        selection_clear();
    }
}

// ---------- table sync screen ----------
lv_obj_t *screen_table_sync = NULL;
static lv_obj_t *table_sync_action_lbl; /* Start <-> Invite quadrant */
static lv_obj_t *table_sync_status_lbl; /* status tile */
static lv_timer_t *table_sync_timer;
static bool table_sync_radio_error;

void refresh_table_sync_ui(void)
{
    static char status_buf[24];
    int status = net_sync_status();
    int code = net_sync_code();

    switch (status) {
        case NET_SYNC_JOINING:
            snprintf(status_buf, sizeof(status_buf), "Joining...");
            break;
        case NET_SYNC_HOSTING:
            snprintf(status_buf, sizeof(status_buf), "Inviting\n#%04d", code);
            break;
        case NET_SYNC_IN_GAME:
            snprintf(status_buf, sizeof(status_buf), "In Game\n#%04d", code);
            break;
        default:
            /* Sync is mirror-mode: a 1p view can't represent the shared
               game, so pairing refuses below and the tile says why. */
            if (nvs_get_players_to_track() <= 1)
                snprintf(status_buf, sizeof(status_buf), "1P View:\nNo Sync");
            else
                snprintf(status_buf, sizeof(status_buf),
                         table_sync_radio_error ? "Radio\nError" : "Sync Off");
            break;
    }
    lv_label_set_text(table_sync_status_lbl, status_buf);
    /* In a game the host action re-opens the invite window for the same
       session (late joiners, rebooted devices) instead of re-keying. */
    lv_label_set_text(table_sync_action_lbl,
        (status == NET_SYNC_HOSTING || status == NET_SYNC_IN_GAME)
            ? "Hold to\nInvite" : "Hold to\nStart");
}

static void event_table_sync_start(lv_event_t *e)
{
    (void)e;
    if (nvs_get_players_to_track() <= 1) return;
    table_sync_radio_error = !net_sync_start_game();
    refresh_table_sync_ui();
}

static void event_table_sync_join(lv_event_t *e)
{
    (void)e;
    if (nvs_get_players_to_track() <= 1) return;
    table_sync_radio_error = !net_sync_join_game();
    refresh_table_sync_ui();
}

static void event_table_sync_leave(lv_event_t *e)
{
    (void)e;
    net_sync_leave_game();
    table_sync_radio_error = false;
    refresh_table_sync_ui();
}

/* Pairing runs in the background (invite window, join listening), so the
   status tile has to track it while the screen is up. The timer pauses
   itself when the user navigates away and is resumed on open. */
static void table_sync_timer_cb(lv_timer_t *timer)
{
    if (lv_scr_act() != screen_table_sync) {
        lv_timer_pause(timer);
        return;
    }
    refresh_table_sync_ui();
}

void open_table_sync_screen(void)
{
    refresh_table_sync_ui();
    lv_timer_resume(table_sync_timer);
    lv_scr_load(screen_table_sync);
}

void build_table_sync_screen(void)
{
    quad_item_t items[4];

    /* All three actions are destructive to a live game (Start opens or
       re-invites, Join drops the current session, Leave exits), so they
       require a long press. */
    memset(items, 0, sizeof(items));
    items[0].label = "Hold to\nStart";
    items[0].cb = event_table_sync_start;
    items[0].enabled = true;
    items[0].event = LV_EVENT_LONG_PRESSED;
    items[1].label = "Hold to\nJoin";
    items[1].cb = event_table_sync_join;
    items[1].enabled = true;
    items[1].event = LV_EVENT_LONG_PRESSED;
    items[2].label = "Hold to\nLeave";
    items[2].cb = event_table_sync_leave;
    items[2].enabled = true;
    items[2].event = LV_EVENT_LONG_PRESSED;
    items[3].label = "Sync Off"; /* status tile, refreshed live */
    items[3].enabled = false;
    items[3].event = LV_EVENT_CLICKED;

    build_quad_screen(&screen_table_sync, items);
    table_sync_action_lbl =
        lv_obj_get_child(lv_obj_get_child(screen_table_sync, 0), 0);
    table_sync_status_lbl =
        lv_obj_get_child(lv_obj_get_child(screen_table_sync, 3), 0);
    table_sync_timer = lv_timer_create(table_sync_timer_cb, 500, NULL);
    lv_timer_pause(table_sync_timer);
}

static const setting_item_t settings_items[] = {
    { .id = "brightness",     .fixed_label = "Brightness", .navigate = open_settings_screen, .nav_screen = &screen_settings },
    { .id = "autodim",        .label = autodim_label,          .color = autodim_color,     .get = autodim_get,              .set = autodim_set,              .count = AUTO_DIM_COUNT },
    { .id = "battery",        .fixed_label = "Battery",    .navigate = open_battery_screen, .nav_screen = &screen_battery },
    { .id = "color-mode",     .label = color_mode_label,       .color = color_mode_color,  .get = nvs_get_color_mode,       .set = nvs_set_color_mode,       .count = COLOR_MODE_COUNT },
    { .id = "deselect",       .label = deselect_label,         .color = deselect_color,    .get = nvs_get_deselect_timeout, .set = nvs_set_deselect_timeout, .count = DESELECT_COUNT },
    { .id = "orientation",    .label = orientation_mode_label, .color = orientation_color, .get = nvs_get_orientation,      .set = nvs_set_orientation,      .count = ORIENTATION_MODE_COUNT },
    { .id = "auto-eliminate", .label = auto_eliminate_label,   .color = toggle_color,      .get = nvs_get_auto_eliminate,   .set = nvs_set_auto_eliminate,   .count = 2 },
    { .id = "random-first",   .label = random_first_label,     .color = toggle_color,      .get = nvs_get_random_first,     .set = nvs_set_random_first,     .count = 2 },
    { .id = "multi-select",   .label = multi_select_label,     .color = toggle_color,      .get = nvs_get_multi_select,     .set = multi_select_set,         .count = 2 },
    { .id = "table-sync",     .fixed_label = "Table Sync\n(Experimental)", .navigate = open_table_sync_screen, .nav_screen = &screen_table_sync },
    { .id = "rotate",         .fixed_label = "Rotate\nScreen", .navigate = open_rotate_screen, .nav_screen = &screen_rotate },
    { .id = "menu-facing",    .label = menu_facing_label,      .color = toggle_color,      .get = nvs_get_menu_facing,      .set = nvs_set_menu_facing,      .count = 2 },
};
#define SETTINGS_ITEM_COUNT ((int)(sizeof(settings_items) / sizeof(settings_items[0])))
#define MAX_SETTINGS_PAGES  ((SETTINGS_ITEM_COUNT + 2) / 3)

lv_obj_t *settings_pages[MAX_SETTINGS_PAGES];
int settings_page_count = 0;
static lv_obj_t *setting_btns[SETTINGS_ITEM_COUNT];
static lv_obj_t *setting_lbls[SETTINGS_ITEM_COUNT];
static int setting_page_of[SETTINGS_ITEM_COUNT];

void refresh_settings_pages_ui(void)
{
    int i;
    for (i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        const setting_item_t *it = &settings_items[i];
        int v;
        if (setting_btns[i] == NULL || it->get == NULL) continue;
        v = it->get();
        lv_label_set_text(setting_lbls[i], it->label(v));
        set_btn_color(setting_btns[i], it->color ? it->color(v) : 0x1A1A2E);
    }
}

static void event_setting_item(lv_event_t *e)
{
    const setting_item_t *it = lv_event_get_user_data(e);
    if (it == NULL) return;
    if (it->navigate != NULL) {
        it->navigate();
        return;
    }
    it->set((it->get() + 1) % it->count);
    refresh_settings_pages_ui();
}

static void event_setting_more(lv_event_t *e)
{
    int page = (int)(intptr_t)lv_event_get_user_data(e);
    if (page >= 0 && page < settings_page_count)
        lv_scr_load(settings_pages[page]);
}

/* Chunk the flat item list into quad pages: 3 items + "More" per page.
   "More" always advances and wraps from the last page to the first, so
   tap navigation cycles just like the knob. */
static void build_settings_pages(void)
{
    int idx = 0;
    int page = 0;
    int total_pages = (SETTINGS_ITEM_COUNT + 2) / 3;

    while (idx < SETTINGS_ITEM_COUNT) {
        int remaining = SETTINGS_ITEM_COUNT - idx;
        int on_page = (remaining < 3) ? remaining : 3;
        int first = idx;
        int s;
        quad_item_t q[4];

        memset(q, 0, sizeof(q));
        for (s = 0; s < 4; s++) q[s].label = "";
        for (s = 0; s < on_page; s++, idx++) {
            const setting_item_t *it = &settings_items[idx];
            q[s].label = (it->label != NULL) ? it->label(it->get()) : it->fixed_label;
            q[s].cb = event_setting_item;
            q[s].enabled = true;
            q[s].event = (it->event != 0) ? it->event : LV_EVENT_CLICKED;
            q[s].user_data = (void *)it;
            setting_page_of[idx] = page;
        }
        q[3].label = "More";
        q[3].cb = event_setting_more;
        q[3].enabled = true;
        q[3].event = LV_EVENT_CLICKED;
        q[3].user_data = (void *)(intptr_t)((page + 1) % total_pages);
        build_quad_screen(&settings_pages[page], q);
        for (s = 0; s < on_page; s++) {
            setting_btns[first + s] = lv_obj_get_child(settings_pages[page], s);
            setting_lbls[first + s] = lv_obj_get_child(setting_btns[first + s], 0);
        }
        page++;
    }
    settings_page_count = page;
    refresh_settings_pages_ui();
}

bool settings_handle_back(lv_obj_t *screen)
{
    int i;

    for (i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        if (settings_items[i].nav_screen != NULL && screen == *settings_items[i].nav_screen) {
            if (screen == screen_settings || screen == screen_rotate) settings_save();
            lv_scr_load(settings_pages[setting_page_of[i]]);
            return true;
        }
    }
    /* Back from any settings page exits to the quad menu — back means
       "leave settings", not "previous page" (the knob flips pages). */
    for (i = 0; i < settings_page_count; i++) {
        if (screen == settings_pages[i]) {
            settings_save();
            lv_scr_load(screen_quad_menu);
            return true;
        }
    }
    return false;
}

/* Knob left/right flips between settings pages, with wraparound.
   Returns false when the active screen is not a settings page. */
bool settings_knob_page(int dir)
{
    int i;

    for (i = 0; i < settings_page_count; i++) {
        if (lv_scr_act() == settings_pages[i]) {
            lv_scr_load(settings_pages[(i + dir + settings_page_count) % settings_page_count]);
            return true;
        }
    }
    return false;
}

int settings_item_page(const char *id)
{
    int i;
    for (i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        if (strcmp(settings_items[i].id, id) == 0)
            return setting_page_of[i];
    }
    return -1;
}

static void event_quad_tools(lv_event_t *e)
{
    (void)e;
    lv_scr_load(screen_tools_menu);
}

static void event_general_game_mode(lv_event_t *e)
{
    (void)e;
    open_game_mode_menu();
}

static void event_open_damage_log(lv_event_t *e)
{
    (void)e;
    open_damage_log_screen();
}

static void event_general_reset(lv_event_t *e)
{
    (void)e;
    reset_all_values();
    back_to_main();
    lv_indev_wait_release(lv_indev_get_act());
}

// ---------- screen builders ----------
void build_quad_menus(void)
{
    quad_item_t main_items[4] = {
        {"Settings", event_quad_screen_settings, true, LV_EVENT_CLICKED},
        {"Game\nMode", event_general_game_mode, true, LV_EVENT_CLICKED},
        {"Tools",             event_quad_tools, true, LV_EVENT_CLICKED},
        {"Reset\n(Hold)", event_general_reset, true, LV_EVENT_LONG_PRESSED},
    };
    build_quad_screen(&screen_quad_menu, main_items);

    quad_item_t tools_items[4] = {
        {"Dice",        event_tool_dice, true, LV_EVENT_CLICKED},
        {"Timer",       event_tool_timer, true, LV_EVENT_CLICKED},
        {"Event\nLog",  event_open_damage_log, true, LV_EVENT_CLICKED},
        {"Mana\nPool",  event_tool_mana, true, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_tools_menu, tools_items);

    build_settings_pages();
}

void build_settings_screen(void)
{
    screen_settings = lv_obj_create(NULL);
    lv_obj_set_size(screen_settings, 360, 360);
    lv_obj_set_style_bg_color(screen_settings, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_settings, 0, 0);
    lv_obj_set_scrollbar_mode(screen_settings, LV_SCROLLBAR_MODE_OFF);

    arc_brightness = lv_arc_create(screen_settings);
    lv_obj_set_size(arc_brightness, 280, 280);
    lv_obj_align(arc_brightness, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(arc_brightness, 90);
    lv_arc_set_bg_angles(arc_brightness, 0, 360);
    lv_arc_set_range(arc_brightness, 0, 100);
    lv_arc_set_value(arc_brightness, brightness_percent);
    lv_obj_remove_style(arc_brightness, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_brightness, LV_OBJ_FLAG_CLICKABLE);

    label_settings_value = lv_label_create(screen_settings);
    lv_label_set_text(label_settings_value, "Brightness: 80%");
    lv_obj_set_style_text_color(label_settings_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_settings_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_settings_value, LV_ALIGN_CENTER, 0, -14);

    label_settings_hint = lv_label_create(screen_settings);
    lv_label_set_text(label_settings_hint, "Turn knob for brightness");
    lv_obj_set_style_text_color(label_settings_hint, lv_color_hex(0x6A6A6A), 0);
    lv_obj_set_style_text_font(label_settings_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_settings_hint, LV_ALIGN_CENTER, 0, 24);
}

void build_battery_screen(void)
{
    screen_battery = lv_obj_create(NULL);
    lv_obj_set_size(screen_battery, 360, 360);
    lv_obj_set_style_bg_color(screen_battery, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_battery, 0, 0);
    lv_obj_set_scrollbar_mode(screen_battery, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen_battery);
    lv_label_set_text(title, "Battery");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    label_settings_battery = lv_label_create(screen_battery);
    lv_label_set_text(label_settings_battery, "Battery: --%");
    lv_obj_set_style_text_color(label_settings_battery, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_settings_battery, &lv_font_montserrat_32, 0);
    lv_obj_align(label_settings_battery, LV_ALIGN_CENTER, 0, -10);

    label_settings_battery_detail = lv_label_create(screen_battery);
    lv_label_set_text(label_settings_battery_detail, "No calibrated reading");
    lv_obj_set_style_text_color(label_settings_battery_detail, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_settings_battery_detail, &lv_font_montserrat_16, 0);
    lv_obj_align(label_settings_battery_detail, LV_ALIGN_CENTER, 0, 30);
}

void build_rotate_screen(void)
{
    screen_rotate = lv_obj_create(NULL);
    lv_obj_set_size(screen_rotate, 360, 360);
    lv_obj_set_style_bg_color(screen_rotate, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_rotate, 0, 0);
    lv_obj_set_scrollbar_mode(screen_rotate, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen_rotate);
    lv_label_set_text(title, "Rotate Screen");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    label_rotate_value = lv_label_create(screen_rotate);
    lv_label_set_text(label_rotate_value, "0°");
    lv_obj_set_style_text_color(label_rotate_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_rotate_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_rotate_value, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *hint = lv_label_create(screen_rotate);
    lv_label_set_text(hint, "Turn knob to rotate");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x6A6A6A), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 40);
}
