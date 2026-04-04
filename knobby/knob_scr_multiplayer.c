#include "knob_scr_multiplayer.h"
#include "knob_scr_main.h"
#include "knob_life.h"
#include "knob_nvs.h"

// ---------- screens ----------
lv_obj_t *screen_multiplayer = NULL;
lv_obj_t *screen_multiplayer_2p = NULL;
lv_obj_t *screen_multiplayer_3p = NULL;
lv_obj_t *screen_multiplayer_menu = NULL;
lv_obj_t *screen_multiplayer_name = NULL;
lv_obj_t *screen_multiplayer_all_damage = NULL;

// ---------- widgets ----------
static lv_obj_t *multiplayer_quadrants[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_life[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_name[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_menu_title = NULL;
static lv_obj_t *label_multiplayer_name_title = NULL;
static lv_obj_t *textarea_multiplayer_name = NULL;
static lv_obj_t *keyboard_multiplayer_name = NULL;
static lv_obj_t *label_multiplayer_all_damage_title = NULL;
static lv_obj_t *label_multiplayer_all_damage_value = NULL;
static lv_obj_t *label_multiplayer_all_damage_hint = NULL;

// ---------- 2-player widgets ----------
static lv_obj_t *mp2_panels[2];
static lv_obj_t *label_mp2_life[2];
static lv_obj_t *label_mp2_name[2];

// ---------- 3-player widgets (reuses 4p layout, slot 3 empty) ----------

// ---------- forward declarations ----------
static void open_multiplayer_name_screen(void);
static void open_multiplayer_all_damage_screen(void);

// ---------- refresh helpers ----------
static void refresh_mp_panel(lv_obj_t *panel, lv_obj_t *life_lbl, lv_obj_t *name_lbl, int i)
{
    char buf[8];
    bool preview_here = multiplayer_life_preview_active && (multiplayer_preview_player == i);
    lv_color_t text_color = get_player_text_color(i);

    if (panel != NULL) {
        lv_obj_set_style_bg_color(panel,
            (i == multiplayer_selected) ? get_player_active_color(i) : get_player_base_color(i), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    }

    if (life_lbl != NULL) {
        if (preview_here) {
            snprintf(buf, sizeof(buf), "%+d", multiplayer_pending_life_delta);
            lv_label_set_text(life_lbl, buf);
            lv_obj_set_style_text_color(life_lbl, get_player_preview_color(i, multiplayer_pending_life_delta), 0);
        } else {
            snprintf(buf, sizeof(buf), "%d", multiplayer_life[i]);
            lv_label_set_text(life_lbl, buf);
            lv_obj_set_style_text_color(life_lbl, text_color, 0);
        }
    }

    if (name_lbl != NULL) {
        lv_label_set_text(name_lbl, multiplayer_names[i]);
        lv_obj_set_style_text_color(name_lbl, text_color, 0);
    }
}

static void refresh_multiplayer_4p_ui(void)
{
    static const int16_t life_offsets_x[MULTIPLAYER_COUNT] = {-90, 90, 90, -90};
    static const int16_t life_offsets_y[MULTIPLAYER_COUNT] = {-90, -90, 90, 90};
    static const int16_t name_offsets_x[MULTIPLAYER_COUNT] = {-90, 90, 90, -90};
    static const int16_t name_offsets_y[MULTIPLAYER_COUNT] = {-40, -40, 32, 32};
    int i;

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        refresh_mp_panel(multiplayer_quadrants[i], label_multiplayer_life[i], label_multiplayer_name[i], i);
        if (label_multiplayer_life[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_life[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_life[i], LV_ALIGN_CENTER, life_offsets_x[i], life_offsets_y[i]);
        }
        if (label_multiplayer_name[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_name[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, name_offsets_x[i], name_offsets_y[i]);
        }
    }
}

static void refresh_multiplayer_2p_ui(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        refresh_mp_panel(mp2_panels[i], label_mp2_life[i], label_mp2_name[i], i);
    }
}

static void refresh_multiplayer_3p_ui(void)
{
    static const int16_t life_offsets_x[MULTIPLAYER_COUNT] = {-90, 90, 90, -90};
    static const int16_t life_offsets_y[MULTIPLAYER_COUNT] = {-90, -90, 90, 90};
    static const int16_t name_offsets_x[MULTIPLAYER_COUNT] = {-90, 90, 90, -90};
    static const int16_t name_offsets_y[MULTIPLAYER_COUNT] = {-40, -40, 32, 32};
    /* Map 3 players to quadrants: 0=top-left, 1=top-right, 3=bottom-left */
    static const int slots[3] = {0, 1, 3};
    int i, q;

    for (i = 0; i < 3; i++) {
        q = slots[i];
        refresh_mp_panel(multiplayer_quadrants[q], label_multiplayer_life[q], label_multiplayer_name[q], i);
        if (label_multiplayer_life[q] != NULL)
            lv_obj_align(label_multiplayer_life[q], LV_ALIGN_CENTER, life_offsets_x[q], life_offsets_y[q]);
        if (label_multiplayer_name[q] != NULL)
            lv_obj_align(label_multiplayer_name[q], LV_ALIGN_CENTER, name_offsets_x[q], name_offsets_y[q]);
    }

    /* Hide bottom-right quadrant (index 2) */
    if (multiplayer_quadrants[2] != NULL)
        lv_obj_set_style_bg_opa(multiplayer_quadrants[2], LV_OPA_TRANSP, 0);
    if (label_multiplayer_life[2] != NULL)
        lv_obj_add_flag(label_multiplayer_life[2], LV_OBJ_FLAG_HIDDEN);
    if (label_multiplayer_name[2] != NULL)
        lv_obj_add_flag(label_multiplayer_name[2], LV_OBJ_FLAG_HIDDEN);
}

// ---------- refresh functions ----------
void refresh_multiplayer_ui(void)
{
    int track = nvs_get_players_to_track();
    if (track == 2) { refresh_multiplayer_2p_ui(); return; }
    if (track == 3) { refresh_multiplayer_3p_ui(); return; }
    refresh_multiplayer_4p_ui();
}

void refresh_multiplayer_menu_ui(void)
{
    char buf[32];

    if (label_multiplayer_menu_title == NULL) return;

    snprintf(buf, sizeof(buf), "%s menu", multiplayer_names[multiplayer_menu_player]);
    lv_label_set_text(label_multiplayer_menu_title, buf);
}

void refresh_multiplayer_name_ui(void)
{
    char buf[40];

    if (label_multiplayer_name_title != NULL) {
        snprintf(buf, sizeof(buf), "Rename %s", multiplayer_names[multiplayer_menu_player]);
        lv_label_set_text(label_multiplayer_name_title, buf);
    }

    if (textarea_multiplayer_name != NULL) {
        lv_textarea_set_text(textarea_multiplayer_name, multiplayer_names[multiplayer_menu_player]);
    }
}

void refresh_multiplayer_all_damage_ui(void)
{
    char buf[32];

    if (label_multiplayer_all_damage_title != NULL) {
        lv_label_set_text(label_multiplayer_all_damage_title, "All players");
    }

    if (label_multiplayer_all_damage_value != NULL) {
        snprintf(buf, sizeof(buf), "Damage: %d", multiplayer_all_damage_value);
        lv_label_set_text(label_multiplayer_all_damage_value, buf);
    }
}

// ---------- navigation ----------
void open_multiplayer_screen(void)
{
    int track = nvs_get_players_to_track();
    refresh_multiplayer_ui();
    if (track == 2) load_screen_if_needed(screen_multiplayer_2p);
    else load_screen_if_needed(screen_multiplayer);
}

void open_multiplayer_menu_screen(int player_index)
{
    multiplayer_menu_player = player_index;
    refresh_multiplayer_menu_ui();
    load_screen_if_needed(screen_multiplayer_menu);
}

static void open_multiplayer_name_screen(void)
{
    refresh_multiplayer_name_ui();
    load_screen_if_needed(screen_multiplayer_name);
}

static void open_multiplayer_all_damage_screen(void)
{
    multiplayer_all_damage_value = 0;
    refresh_multiplayer_all_damage_ui();
    load_screen_if_needed(screen_multiplayer_all_damage);
}

// ---------- quadrant-to-player mapping ----------
static int quad_to_player(int quad)
{
    int track = nvs_get_players_to_track();
    if (track == 3) {
        if (quad == 2) return -1;  /* bottom-right is empty */
        if (quad == 3) return 2;   /* bottom-left is player 2 */
    } else {
        if (quad >= track) return -1;
    }
    return quad;
}

// ---------- events ----------
static void event_multiplayer_select(lv_event_t *e)
{
    int quad = (int)(intptr_t)lv_event_get_user_data(e);
    int player = quad_to_player(quad);

    if (player < 0) return;

    if (multiplayer_life_preview_active && multiplayer_preview_player != player) {
        multiplayer_life_preview_commit_cb(NULL);
    }

    multiplayer_selected = player;
    refresh_multiplayer_ui();
}

static void event_multiplayer_open_menu(lv_event_t *e)
{
    int quad = (int)(intptr_t)lv_event_get_user_data(e);
    int player = quad_to_player(quad);

    if (player < 0) return;

    if (multiplayer_life_preview_active && multiplayer_preview_player != player) {
        multiplayer_life_preview_commit_cb(NULL);
    }

    multiplayer_selected = player;
    refresh_multiplayer_ui();
    open_multiplayer_menu_screen(multiplayer_selected);
}

static void event_multiplayer_menu_rename(lv_event_t *e)
{
    (void)e;
    open_multiplayer_name_screen();
}

static void event_multiplayer_menu_cmd_damage(lv_event_t *e)
{
    (void)e;
    prepare_cmd_damage_for_player(multiplayer_menu_player);
    open_select_screen();
}

static void event_multiplayer_menu_all_damage(lv_event_t *e)
{
    (void)e;
    open_multiplayer_all_damage_screen();
}

static void event_multiplayer_name_save(lv_event_t *e)
{
    const char *txt;
    size_t len;

    (void)e;
    if (textarea_multiplayer_name == NULL) return;

    txt = lv_textarea_get_text(textarea_multiplayer_name);
    len = strlen(txt);
    if (len == 0) {
        snprintf(multiplayer_names[multiplayer_menu_player], sizeof(multiplayer_names[multiplayer_menu_player]),
                 "P%d", multiplayer_menu_player + 1);
    } else {
        snprintf(multiplayer_names[multiplayer_menu_player],
                 sizeof(multiplayer_names[multiplayer_menu_player]), "%s", txt);
    }

    refresh_multiplayer_ui();
    refresh_multiplayer_menu_ui();
    refresh_multiplayer_name_ui();
    refresh_select_ui();
    refresh_damage_ui();
    open_multiplayer_menu_screen(multiplayer_menu_player);
}

static void event_multiplayer_all_damage_apply(lv_event_t *e)
{
    int i;

    (void)e;
    for (i = 0; i < nvs_get_players_to_track(); i++) {
        multiplayer_life[i] = clamp_life(multiplayer_life[i] - multiplayer_all_damage_value);
    }

    refresh_multiplayer_ui();
    open_multiplayer_screen();
}

// ---------- screen builders ----------
void build_multiplayer_screen(void)
{
    static const char *player_names[MULTIPLAYER_COUNT] = {"P1", "P2", "P3", "P4"};
    static const lv_coord_t quad_x[MULTIPLAYER_COUNT] = {0, 180, 180, 0};
    static const lv_coord_t quad_y[MULTIPLAYER_COUNT] = {0, 0, 180, 180};
    int i;

    screen_multiplayer = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer, LV_SCROLLBAR_MODE_OFF);

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        multiplayer_quadrants[i] = lv_btn_create(screen_multiplayer);
        lv_obj_set_size(multiplayer_quadrants[i], 180, 180);
        lv_obj_set_pos(multiplayer_quadrants[i], quad_x[i], quad_y[i]);
        lv_obj_set_style_radius(multiplayer_quadrants[i], 0, 0);
        lv_obj_set_style_border_width(multiplayer_quadrants[i], 1, 0);
        lv_obj_set_style_border_color(multiplayer_quadrants[i], lv_color_black(), 0);
        lv_obj_set_style_shadow_width(multiplayer_quadrants[i], 0, 0);
        lv_obj_add_flag(multiplayer_quadrants[i], LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(multiplayer_quadrants[i], event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(multiplayer_quadrants[i], event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);

        label_multiplayer_name[i] = lv_label_create(screen_multiplayer);
        lv_label_set_text(label_multiplayer_name[i], player_names[i]);
        lv_obj_set_style_text_color(label_multiplayer_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_multiplayer_name[i], &lv_font_montserrat_14, 0);

        label_multiplayer_life[i] = lv_label_create(screen_multiplayer);
        lv_label_set_text(label_multiplayer_life[i], "40");
        lv_obj_set_style_text_color(label_multiplayer_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_multiplayer_life[i], &lv_font_montserrat_32, 0);
    }

    refresh_multiplayer_ui();
}

void build_multiplayer_menu_screen(void)
{
    screen_multiplayer_menu = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer_menu, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer_menu, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer_menu, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer_menu, LV_SCROLLBAR_MODE_OFF);

    label_multiplayer_menu_title = lv_label_create(screen_multiplayer_menu);
    lv_obj_set_style_text_color(label_multiplayer_menu_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_menu_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_multiplayer_menu_title, LV_ALIGN_TOP_MID, 0, 26);

    lv_obj_t *btn = make_button(screen_multiplayer_menu, "rename", 180, 46, event_multiplayer_menu_rename);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -36);

    btn = make_button(screen_multiplayer_menu, "Cmd.dmg", 180, 46, event_multiplayer_menu_cmd_damage);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 24);

    btn = make_button(screen_multiplayer_menu, "all.dmg", 180, 46, event_multiplayer_menu_all_damage);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 84);
}

void build_multiplayer_name_screen(void)
{
    screen_multiplayer_name = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer_name, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer_name, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer_name, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer_name, LV_SCROLLBAR_MODE_OFF);

    label_multiplayer_name_title = lv_label_create(screen_multiplayer_name);
    lv_obj_set_style_text_color(label_multiplayer_name_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_name_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_multiplayer_name_title, LV_ALIGN_TOP_MID, 0, 18);

    textarea_multiplayer_name = lv_textarea_create(screen_multiplayer_name);
    lv_obj_set_size(textarea_multiplayer_name, 240, 44);
    lv_obj_align(textarea_multiplayer_name, LV_ALIGN_TOP_MID, 0, 56);
    lv_textarea_set_max_length(textarea_multiplayer_name, 15);
    lv_textarea_set_one_line(textarea_multiplayer_name, true);

    keyboard_multiplayer_name = lv_keyboard_create(screen_multiplayer_name);
    lv_obj_set_size(keyboard_multiplayer_name, 360, 170);
    lv_obj_align(keyboard_multiplayer_name, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard_multiplayer_name, textarea_multiplayer_name);

    lv_obj_t *btn = make_button(screen_multiplayer_name, "save", 88, 38, event_multiplayer_name_save);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 116);
}

void build_multiplayer_all_damage_screen(void)
{
    screen_multiplayer_all_damage = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer_all_damage, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer_all_damage, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer_all_damage, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer_all_damage, LV_SCROLLBAR_MODE_OFF);

    label_multiplayer_all_damage_title = lv_label_create(screen_multiplayer_all_damage);
    lv_obj_set_style_text_color(label_multiplayer_all_damage_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_multiplayer_all_damage_title, LV_ALIGN_TOP_MID, 0, 26);

    label_multiplayer_all_damage_value = lv_label_create(screen_multiplayer_all_damage);
    lv_obj_set_style_text_color(label_multiplayer_all_damage_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_multiplayer_all_damage_value, LV_ALIGN_CENTER, 0, -8);

    label_multiplayer_all_damage_hint = lv_label_create(screen_multiplayer_all_damage);
    lv_label_set_text(label_multiplayer_all_damage_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_multiplayer_all_damage_hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_multiplayer_all_damage_hint, LV_ALIGN_CENTER, 0, 38);

    lv_obj_t *btn = make_button(screen_multiplayer_all_damage, "apply", 120, 46, event_multiplayer_all_damage_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

// ---------- 2-player screen (top/bottom split) ----------
void build_multiplayer_2p_screen(void)
{
    static const lv_coord_t panel_y[2] = {0, 182};
    int i;

    screen_multiplayer_2p = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer_2p, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer_2p, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer_2p, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer_2p, LV_SCROLLBAR_MODE_OFF);

    for (i = 0; i < 2; i++) {
        mp2_panels[i] = lv_btn_create(screen_multiplayer_2p);
        lv_obj_set_size(mp2_panels[i], 360, 178);
        lv_obj_set_pos(mp2_panels[i], 0, panel_y[i]);
        lv_obj_set_style_radius(mp2_panels[i], 0, 0);
        lv_obj_set_style_border_width(mp2_panels[i], 1, 0);
        lv_obj_set_style_border_color(mp2_panels[i], lv_color_black(), 0);
        lv_obj_set_style_shadow_width(mp2_panels[i], 0, 0);
        lv_obj_add_event_cb(mp2_panels[i], event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(mp2_panels[i], event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);

        label_mp2_name[i] = lv_label_create(mp2_panels[i]);
        lv_label_set_text(label_mp2_name[i], multiplayer_names[i]);
        lv_obj_set_style_text_color(label_mp2_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp2_name[i], &lv_font_montserrat_14, 0);
        lv_obj_align(label_mp2_name[i], LV_ALIGN_CENTER, 0, 30);

        label_mp2_life[i] = lv_label_create(mp2_panels[i]);
        lv_label_set_text(label_mp2_life[i], "40");
        lv_obj_set_style_text_color(label_mp2_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp2_life[i], &lv_font_montserrat_32, 0);
        lv_obj_align(label_mp2_life[i], LV_ALIGN_CENTER, 0, -10);
    }
}

// ---------- 3-player screen (reuses 4p layout, bottom-left empty) ----------
void build_multiplayer_3p_screen(void)
{
    /* 3p shares screen_multiplayer with 4p; nothing to build */
    screen_multiplayer_3p = screen_multiplayer;
}
