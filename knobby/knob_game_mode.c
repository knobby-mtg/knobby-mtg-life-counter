#include "knob_game_mode.h"
#include "knob_nvs.h"
#include "knob_scr_menus.h"

// Forward declarations
extern void reset_all_values(void);
extern void back_to_main(void);

// ---------- screens ----------
lv_obj_t *screen_game_mode_menu = NULL;
lv_obj_t *screen_custom_life = NULL;

// ---------- dynamic labels ----------
static lv_obj_t *label_gm_num_players = NULL;
static lv_obj_t *label_gm_players_to_track = NULL;
static lv_obj_t *label_gm_life_total = NULL;

// ---------- custom life widgets ----------
static lv_obj_t *label_custom_life_value = NULL;

// ---------- temp settings (applied on Apply) ----------
static int temp_num_players;
static int temp_players_to_track;
static int temp_life_total;

// ---------- refresh ----------
void refresh_game_mode_menu_ui(void)
{
    char buf[32];
    int max_track;

    snprintf(buf, sizeof(buf), "Players\n%d", temp_num_players);
    lv_label_set_text(label_gm_num_players, buf);

    max_track = temp_num_players < 4 ? temp_num_players : 4;
    if (temp_players_to_track > max_track) temp_players_to_track = max_track;
    snprintf(buf, sizeof(buf), "Track\n%d", temp_players_to_track);
    lv_label_set_text(label_gm_players_to_track, buf);

    snprintf(buf, sizeof(buf), "Life\n%d", temp_life_total);
    lv_label_set_text(label_gm_life_total, buf);
}

void refresh_custom_life_ui(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", temp_life_total);
    lv_label_set_text(label_custom_life_value, buf);
}

// ---------- navigation ----------
void open_game_mode_menu(void)
{
    temp_num_players = nvs_get_num_players();
    temp_players_to_track = nvs_get_players_to_track();
    temp_life_total = nvs_get_life_total();
    refresh_game_mode_menu_ui();
    lv_scr_load(screen_game_mode_menu);
}

// ---------- knob input ----------
void change_custom_life(int delta)
{
    temp_life_total += delta;
    if (temp_life_total < 0) temp_life_total = 0;
    if (temp_life_total > LIFE_MAX) temp_life_total = LIFE_MAX;
    refresh_custom_life_ui();
}

// ---------- events ----------
static void event_gm_num_players(lv_event_t *e)
{
    int max_track;
    (void)e;

    temp_num_players++;
    if (temp_num_players > MAX_PLAYERS) temp_num_players = 1;

    max_track = temp_num_players < 4 ? temp_num_players : 4;
    if (temp_players_to_track > max_track) temp_players_to_track = max_track;

    refresh_game_mode_menu_ui();
}

static void event_gm_players_to_track(lv_event_t *e)
{
    int max_track;
    (void)e;

    max_track = temp_num_players < 4 ? temp_num_players : 4;
    temp_players_to_track++;
    if (temp_players_to_track > max_track) temp_players_to_track = 1;

    refresh_game_mode_menu_ui();
}

static void event_gm_life_cycle(lv_event_t *e)
{
    (void)e;

    if (temp_life_total == 20) temp_life_total = 30;
    else if (temp_life_total == 30) temp_life_total = 40;
    else temp_life_total = 20;

    refresh_game_mode_menu_ui();
}

static void event_gm_life_custom(lv_event_t *e)
{
    (void)e;
    refresh_custom_life_ui();
    lv_scr_load(screen_custom_life);
}

static void event_gm_apply(lv_event_t *e)
{
    (void)e;
    nvs_set_num_players(temp_num_players);
    nvs_set_players_to_track(temp_players_to_track);
    nvs_set_life_total(temp_life_total);
    settings_save();
    reset_all_values();
    back_to_main();
}

// ---------- screen builders ----------
void build_game_mode_menu_screen(void)
{
    lv_obj_t *btn;

    quad_item_t items[4] = {
        {"Players\n4",          event_gm_num_players,      true, LV_EVENT_CLICKED},
        {"Track\n1",            event_gm_players_to_track, true, LV_EVENT_CLICKED},
        {"Life\n40",            event_gm_life_cycle,       true, LV_EVENT_CLICKED},
        {"Apply\n(Long Press)", event_gm_apply,            true, LV_EVENT_LONG_PRESSED},
    };
    build_quad_screen(&screen_game_mode_menu, items);

    // Store label references for dynamic updates
    btn = lv_obj_get_child(screen_game_mode_menu, 0);
    label_gm_num_players = lv_obj_get_child(btn, 0);
    btn = lv_obj_get_child(screen_game_mode_menu, 1);
    label_gm_players_to_track = lv_obj_get_child(btn, 0);
    btn = lv_obj_get_child(screen_game_mode_menu, 2);
    label_gm_life_total = lv_obj_get_child(btn, 0);

    // Add long press on life total for custom value entry
    lv_obj_add_event_cb(btn, event_gm_life_custom, LV_EVENT_LONG_PRESSED, NULL);
}

void build_custom_life_screen(void)
{
    lv_obj_t *title;
    lv_obj_t *hint;

    screen_custom_life = lv_obj_create(NULL);
    lv_obj_set_size(screen_custom_life, 360, 360);
    lv_obj_set_style_bg_color(screen_custom_life, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_custom_life, 0, 0);
    lv_obj_set_scrollbar_mode(screen_custom_life, LV_SCROLLBAR_MODE_OFF);

    title = lv_label_create(screen_custom_life);
    lv_label_set_text(title, "Life Total");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    label_custom_life_value = lv_label_create(screen_custom_life);
    lv_label_set_text(label_custom_life_value, "40");
    lv_obj_set_style_text_color(label_custom_life_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_custom_life_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_custom_life_value, LV_ALIGN_CENTER, 0, -10);

    hint = lv_label_create(screen_custom_life);
    lv_label_set_text(hint, "Turn knob to adjust");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x6A6A6A), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 24);
}
