#include "settings.h"
#include "hw.h"
#include "storage.h"
#include "dice.h"
#include "timer.h"
#include "game_mode.h"
#include "damage_log.h"
#include "rename.h"
#include "game.h"
#include "ui_wireless.h"

// Forward declarations for cross-module calls
extern void reset_all_values(void);
extern void back_to_main(void);

// ---------- screens ----------
lv_obj_t *screen_quad_menu = NULL;
lv_obj_t *screen_tools_menu = NULL;
lv_obj_t *screen_screen_settings_menu = NULL;
lv_obj_t *screen_settings_page2 = NULL;
lv_obj_t *screen_settings_page3 = NULL;
lv_obj_t *screen_settings = NULL;
lv_obj_t *screen_battery = NULL;

// ---------- widgets ----------
static lv_obj_t *btn_autodim = NULL;
static lv_obj_t *label_autodim_quad = NULL;
static lv_obj_t *btn_color_mode = NULL;
static lv_obj_t *label_color_mode_quad = NULL;
static lv_obj_t *btn_deselect = NULL;
static lv_obj_t *label_deselect_quad = NULL;
static lv_obj_t *btn_orientation = NULL;
static lv_obj_t *label_orientation_quad = NULL;
static lv_obj_t *btn_auto_eliminate = NULL;
static lv_obj_t *label_auto_eliminate_quad = NULL;
static lv_obj_t *arc_brightness = NULL;
static lv_obj_t *label_settings_value = NULL;
static lv_obj_t *label_settings_hint = NULL;
static lv_obj_t *label_settings_battery = NULL;
static lv_obj_t *label_settings_battery_detail = NULL;

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
        lv_obj_set_size(btn, 178, 178);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_pos(btn, qx[i], qy[i]);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        if (items[i].cb != NULL && items[i].enabled) {
            lv_obj_add_event_cb(btn, items[i].cb, items[i].event, NULL);
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
    lv_scr_load(screen_screen_settings_menu);
}

static void event_screen_brightness(lv_event_t *e)
{
    (void)e;
    open_settings_screen();
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

static void event_screen_autodim(lv_event_t *e)
{
    (void)e;
    auto_dim_setting = (auto_dim_setting + 1) % AUTO_DIM_COUNT;
    nvs_set_auto_dim(auto_dim_setting);
    if (auto_dim_setting == AUTO_DIM_OFF && dimmed) {
        dimmed = false;
        brightness_apply();
    }
    if (label_autodim_quad) {
        lv_label_set_text(label_autodim_quad, autodim_label(auto_dim_setting));
    }
    set_btn_color(btn_autodim, autodim_color(auto_dim_setting));
}

static const char *color_mode_label(int mode)
{
    switch (mode) {
        case COLOR_MODE_LIFE:   return "Colors\nLife";
        default:                return "Colors\nPlayer";
    }
}

static void event_screen_color_mode(lv_event_t *e)
{
    int mode;
    (void)e;
    mode = (nvs_get_color_mode() + 1) % COLOR_MODE_COUNT;
    nvs_set_color_mode(mode);
    if (label_color_mode_quad) {
        lv_label_set_text(label_color_mode_quad, color_mode_label(mode));
    }
    set_btn_color(btn_color_mode, color_mode_color(mode));
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

static void event_screen_deselect(lv_event_t *e)
{
    int val;
    (void)e;
    val = (nvs_get_deselect_timeout() + 1) % DESELECT_COUNT;
    nvs_set_deselect_timeout(val);
    if (label_deselect_quad) {
        lv_label_set_text(label_deselect_quad, deselect_label(val));
    }
    set_btn_color(btn_deselect, deselect_color(val));
}

static void event_screen_orientation(lv_event_t *e)
{
    int val;
    (void)e;
    val = (nvs_get_orientation() + 1) % ORIENTATION_MODE_COUNT;
    nvs_set_orientation(val);
    if (label_orientation_quad) {
        lv_label_set_text(label_orientation_quad, orientation_mode_label(val));
    }
    set_btn_color(btn_orientation, orientation_color(val));
}

static const char *auto_eliminate_label(int val)
{
    return val ? "Auto\nElimination\nON" : "Auto\nElimination\nOFF";
}

static void event_screen_auto_eliminate(lv_event_t *e)
{
    int val;
    (void)e;
    val = !nvs_get_auto_eliminate();
    nvs_set_auto_eliminate(val);
    if (label_auto_eliminate_quad) {
        lv_label_set_text(label_auto_eliminate_quad, auto_eliminate_label(val));
    }
    set_btn_color(btn_auto_eliminate, val ? TOGGLE_ON : TOGGLE_OFF);
}

static void event_screen_more(lv_event_t *e)
{
    (void)e;
    lv_scr_load(screen_settings_page2);
}

static void event_screen_more_page3(lv_event_t *e)
{
    (void)e;
    lv_scr_load(screen_settings_page3);
}

static void event_screen_wireless(lv_event_t *e)
{
    (void)e;
    open_wireless_menu_screen();
}

static void event_screen_battery(lv_event_t *e)
{
    (void)e;
    update_battery_measurement(true);
    refresh_battery_ui();
    lv_scr_load(screen_battery);
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

static void event_tool_rename_all(lv_event_t *e)
{
    (void)e;
    menu_player = 0;
    open_rename_all_screen();
}

static void event_general_reset(lv_event_t *e)
{
    (void)e;
    damage_log_reset();
    reset_all_values();
    back_to_main();
}

// ---------- screen builders ----------
void build_quad_menus(void)
{
    quad_item_t main_items[4] = {
        {"Settings", event_quad_screen_settings, true, LV_EVENT_CLICKED},
        {"Game\nMode", event_general_game_mode, true, LV_EVENT_CLICKED},
        {"Tools",             event_quad_tools, true, LV_EVENT_CLICKED},
        {"Reset\n(Long Press)", event_general_reset, true, LV_EVENT_LONG_PRESSED},
    };
    build_quad_screen(&screen_quad_menu, main_items);

    quad_item_t tools_items[4] = {
        {"Dice",        event_tool_dice, true, LV_EVENT_CLICKED},
        {"Timer",       event_tool_timer, true, LV_EVENT_CLICKED},
        {"Event\nLog",  event_open_damage_log, true, LV_EVENT_CLICKED},
        {"Rename\nAll", event_tool_rename_all, true, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_tools_menu, tools_items);

    quad_item_t screen_items[4] = {
        {"Brightness", event_screen_brightness, true, LV_EVENT_CLICKED},
        {autodim_label(auto_dim_setting), event_screen_autodim, true, LV_EVENT_CLICKED},
        {"Battery",       event_screen_battery, true, LV_EVENT_CLICKED},
        {"More",          event_screen_more, true, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_screen_settings_menu, screen_items);

    btn_autodim = lv_obj_get_child(screen_screen_settings_menu, 1);
    label_autodim_quad = lv_obj_get_child(btn_autodim, 0);
    set_btn_color(btn_autodim, autodim_color(auto_dim_setting));

    quad_item_t page2_items[4] = {
        {color_mode_label(nvs_get_color_mode()), event_screen_color_mode, true, LV_EVENT_CLICKED},
        {deselect_label(nvs_get_deselect_timeout()), event_screen_deselect, true, LV_EVENT_CLICKED},
        {orientation_mode_label(nvs_get_orientation()), event_screen_orientation, true, LV_EVENT_CLICKED},
        {"More",                              event_screen_more_page3, true, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_settings_page2, page2_items);

    btn_color_mode = lv_obj_get_child(screen_settings_page2, 0);
    label_color_mode_quad = lv_obj_get_child(btn_color_mode, 0);
    set_btn_color(btn_color_mode, color_mode_color(nvs_get_color_mode()));

    btn_deselect = lv_obj_get_child(screen_settings_page2, 1);
    label_deselect_quad = lv_obj_get_child(btn_deselect, 0);
    set_btn_color(btn_deselect, deselect_color(nvs_get_deselect_timeout()));

    btn_orientation = lv_obj_get_child(screen_settings_page2, 2);
    label_orientation_quad = lv_obj_get_child(btn_orientation, 0);
    set_btn_color(btn_orientation, orientation_color(nvs_get_orientation()));

    quad_item_t page3_items[4] = {
        {auto_eliminate_label(nvs_get_auto_eliminate()), event_screen_auto_eliminate, true, LV_EVENT_CLICKED},
        {"Wireless",                          event_screen_wireless, true, LV_EVENT_CLICKED},
        {"",                                  NULL, false, LV_EVENT_CLICKED},
        {"",                                  NULL, false, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_settings_page3, page3_items);

    btn_auto_eliminate = lv_obj_get_child(screen_settings_page3, 0);
    label_auto_eliminate_quad = lv_obj_get_child(btn_auto_eliminate, 0);
    set_btn_color(btn_auto_eliminate, nvs_get_auto_eliminate() ? TOGGLE_ON : TOGGLE_OFF);
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
