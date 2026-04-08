#include "knob_scr_multiplayer.h"
#include "knob_scr_main.h"
#include "knob_life.h"
#include "knob_nvs.h"
#include "knob_damage_log.h"
#include "knob_rename.h"
#include "knob_scr_menus.h"

// ---------- screens ----------
lv_obj_t *screen_4p = NULL;
lv_obj_t *screen_2p = NULL;
lv_obj_t *screen_3p = NULL;
lv_obj_t *screen_player_menu = NULL;
lv_obj_t *screen_player_all_damage = NULL;

// ---------- widgets ----------
static lv_obj_t *multiplayer_quadrants[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_life[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_name[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_all_damage_title = NULL;
static lv_obj_t *label_multiplayer_all_damage_value = NULL;
static lv_obj_t *label_multiplayer_all_damage_hint = NULL;

// ---------- 2-player widgets ----------
static lv_obj_t *mp2_panels[2];
static lv_obj_t *label_mp2_life[2];
static lv_obj_t *label_mp2_name[2];

// ---------- selection auto-deselect ----------
static lv_timer_t *select_timeout_timer = NULL;

// ---------- 3-player widgets (reuses 4p layout, slot 3 empty) ----------

// ---------- forward declarations ----------
static void open_multiplayer_all_damage_screen(void);

// ---------- rotation helper ----------
static void apply_label_rotation(lv_obj_t *life_lbl, lv_obj_t *name_lbl,
                                  int16_t angle, int life_pivot_y, int name_pivot_y)
{
    if (life_lbl != NULL) {
        lv_obj_set_style_transform_angle(life_lbl, angle, 0);
        if (angle != 0) {
            lv_obj_update_layout(life_lbl);
            lv_obj_set_style_transform_pivot_x(life_lbl,
                lv_obj_get_width(life_lbl) / 2, 0);
            lv_obj_set_style_transform_pivot_y(life_lbl,
                lv_obj_get_height(life_lbl) / 2 + life_pivot_y, 0);
        }
    }
    if (name_lbl != NULL) {
        lv_obj_set_style_transform_angle(name_lbl, angle, 0);
        if (angle != 0) {
            lv_obj_update_layout(name_lbl);
            lv_obj_set_style_transform_pivot_x(name_lbl,
                lv_obj_get_width(name_lbl) / 2, 0);
            lv_obj_set_style_transform_pivot_y(name_lbl,
                lv_obj_get_height(name_lbl) / 2 + name_pivot_y, 0);
        }
    }
}

// ---------- refresh helpers ----------
static void refresh_mp_panel(lv_obj_t *panel, lv_obj_t *life_lbl, lv_obj_t *name_lbl, int i, int color_i)
{
    char buf[8];
    bool preview_here = multiplayer_life_preview_active && (multiplayer_preview_player == i);
    bool selected = (i == multiplayer_selected);
    lv_color_t bg_color;
    lv_color_t text_color;

    if (nvs_get_color_mode() == COLOR_MODE_LIFE) {
        int tier = get_life_tier(multiplayer_life[i], nvs_get_life_total());
        int vib;
        if (multiplayer_selected < 0) vib = LIFE_VIB_MID;
        else vib = selected ? LIFE_VIB_VIV : LIFE_VIB_DIM;
        bg_color = get_life_color_vib(tier, vib);
        text_color = color_is_light(bg_color) ? lv_color_black() : lv_color_white();
    } else {
        int vib;
        if (multiplayer_selected < 0) vib = LIFE_VIB_MID;
        else vib = selected ? LIFE_VIB_VIV : LIFE_VIB_DIM;
        bg_color = get_player_color_vib(color_i, vib);
        text_color = color_is_light(bg_color) ? lv_color_black() : lv_color_white();
    }

    if (panel != NULL) {
        lv_obj_set_style_bg_color(panel, bg_color, 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    }

    if (life_lbl != NULL) {
        if (preview_here) {
            snprintf(buf, sizeof(buf), "%+d", multiplayer_pending_life_delta);
            lv_label_set_text(life_lbl, buf);
            {
                lv_color_t preview_c;
                if (nvs_get_color_mode() == COLOR_MODE_LIFE) {
                    /* Life-color mode: pick purely on bg contrast */
                    preview_c = color_is_light(bg_color) ? lv_color_black() : lv_color_white();
                } else {
                    preview_c = get_player_preview_color(color_i, multiplayer_pending_life_delta);
                    /* If preview color would blend into bg, flip */
                    if (color_is_light(bg_color) && color_is_light(preview_c))
                        preview_c = lv_color_black();
                    else if (!color_is_light(bg_color) && !color_is_light(preview_c))
                        preview_c = lv_color_white();
                }
                lv_obj_set_style_text_color(life_lbl, preview_c, 0);
            }
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
    static const int16_t rot[MULTIPLAYER_COUNT] = {450, 1350, 2250, 3150};
    bool do_rot = nvs_get_rotation();
    int i;

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        refresh_mp_panel(multiplayer_quadrants[i], label_multiplayer_life[i], label_multiplayer_name[i], i, i);
        if (label_multiplayer_life[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_life[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_life[i], LV_ALIGN_CENTER, 0, do_rot ? -30 : -12);
        }
        if (label_multiplayer_name[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_name[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, 0, 30);
        }
        apply_label_rotation(label_multiplayer_life[i], label_multiplayer_name[i],
            do_rot ? rot[i] : 0, 30, -30);
    }
}

static void refresh_multiplayer_2p_ui(void)
{
    /* Panel 0 = top = P2 (player 1), Panel 1 = bottom = P1 (player 0) */
    static const int panel_player[2] = {1, 0};
    static const int panel_color[2]  = {0, 1};
    bool do_rot = nvs_get_rotation();
    int i;
    for (i = 0; i < 2; i++) {
        refresh_mp_panel(mp2_panels[i], label_mp2_life[i], label_mp2_name[i],
                         panel_player[i], panel_color[i]);
        apply_label_rotation(label_mp2_life[i], label_mp2_name[i],
            (i == 0 && do_rot) ? 1800 : 0, 10, -30);
    }
}

static void refresh_multiplayer_3p_ui(void)
{
    static const int16_t rot[MULTIPLAYER_COUNT] = {450, 1350, 2250, 3150};
    bool do_rot = nvs_get_rotation();
    int i;

    for (i = 0; i < 3; i++) {
        refresh_mp_panel(multiplayer_quadrants[i], label_multiplayer_life[i], label_multiplayer_name[i], i, i);
        if (label_multiplayer_life[i] != NULL)
            lv_obj_align(label_multiplayer_life[i], LV_ALIGN_CENTER, 0, do_rot ? -30 : -12);
        if (label_multiplayer_name[i] != NULL)
            lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, 0, 30);
        apply_label_rotation(label_multiplayer_life[i], label_multiplayer_name[i],
            do_rot ? rot[i] : 0, 30, -30);
    }

    /* Hide bottom-right quadrant (index 3) */
    if (multiplayer_quadrants[3] != NULL)
        lv_obj_set_style_bg_opa(multiplayer_quadrants[3], LV_OPA_TRANSP, 0);
    if (label_multiplayer_life[3] != NULL)
        lv_obj_add_flag(label_multiplayer_life[3], LV_OBJ_FLAG_HIDDEN);
    if (label_multiplayer_name[3] != NULL)
        lv_obj_add_flag(label_multiplayer_name[3], LV_OBJ_FLAG_HIDDEN);
}

// ---------- refresh functions ----------
void refresh_multiplayer_ui(void)
{
    int track = nvs_get_players_to_track();
    if (track == 2) { refresh_multiplayer_2p_ui(); return; }
    if (track == 3) { refresh_multiplayer_3p_ui(); return; }
    refresh_multiplayer_4p_ui();
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
    if (track == 2) load_screen_if_needed(screen_2p);
    else load_screen_if_needed(screen_4p);
}

void open_multiplayer_menu_screen(int player_index)
{
    multiplayer_menu_player = player_index;
    load_screen_if_needed(screen_player_menu);
}

static void open_multiplayer_all_damage_screen(void)
{
    multiplayer_all_damage_value = 0;
    refresh_multiplayer_all_damage_ui();
    load_screen_if_needed(screen_player_all_damage);
}

// ---------- quadrant-to-player mapping ----------
static int quad_to_player(int quad)
{
    int track = nvs_get_players_to_track();
    if (quad >= track) return -1;
    return quad;
}

// ---------- selection timeout ----------
static void select_timeout_cb(lv_timer_t *timer)
{
    (void)timer;
    multiplayer_selected = -1;
    if (select_timeout_timer != NULL)
        lv_timer_pause(select_timeout_timer);
    refresh_multiplayer_ui();
}

void select_kick_timer(void)
{
    int idx = nvs_get_deselect_timeout();
    int ms = deselect_ms[idx];

    if (select_timeout_timer == NULL) {
        select_timeout_timer = lv_timer_create(select_timeout_cb, 15000, NULL);
        lv_timer_pause(select_timeout_timer);
    }
    if (multiplayer_selected >= 0 && ms > 0) {
        lv_timer_set_period(select_timeout_timer, (uint32_t)ms);
        lv_timer_reset(select_timeout_timer);
        lv_timer_resume(select_timeout_timer);
    } else {
        lv_timer_pause(select_timeout_timer);
    }
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

    if (multiplayer_selected == player) {
        multiplayer_selected = -1;
    } else {
        multiplayer_selected = player;
    }
    select_kick_timer();
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
    open_rename_screen();
}

static void event_multiplayer_menu_rename_all(lv_event_t *e)
{
    (void)e;
    open_rename_all_screen();
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

static void event_multiplayer_all_damage_apply(lv_event_t *e)
{
    int i;

    (void)e;
    for (i = 0; i < nvs_get_players_to_track(); i++) {
        damage_log_add(i, -multiplayer_all_damage_value);
        multiplayer_life[i] = clamp_life(multiplayer_life[i] - multiplayer_all_damage_value);
    }

    refresh_multiplayer_ui();
    open_multiplayer_screen();
}

// ---------- screen builders ----------
void build_multiplayer_screen(void)
{
    static const char *player_names[MULTIPLAYER_COUNT] = {"P1", "P2", "P3", "P4"};
    static const lv_coord_t quad_x[MULTIPLAYER_COUNT] = {0, 0, 180, 180};
    static const lv_coord_t quad_y[MULTIPLAYER_COUNT] = {180, 0, 0, 180};
    int i;

    screen_4p = lv_obj_create(NULL);
    lv_obj_set_size(screen_4p, 360, 360);
    lv_obj_set_style_bg_color(screen_4p, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_4p, 0, 0);
    lv_obj_set_scrollbar_mode(screen_4p, LV_SCROLLBAR_MODE_OFF);

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        multiplayer_quadrants[i] = lv_btn_create(screen_4p);
        lv_obj_remove_style_all(multiplayer_quadrants[i]);
        lv_obj_set_style_bg_opa(multiplayer_quadrants[i], LV_OPA_COVER, 0);
        lv_obj_set_size(multiplayer_quadrants[i], 180, 180);
        lv_obj_set_pos(multiplayer_quadrants[i], quad_x[i], quad_y[i]);
        lv_obj_set_style_radius(multiplayer_quadrants[i], 0, 0);
        lv_obj_set_style_border_width(multiplayer_quadrants[i], 1, 0);
        lv_obj_set_style_border_color(multiplayer_quadrants[i], lv_color_black(), 0);
        lv_obj_set_style_shadow_width(multiplayer_quadrants[i], 0, 0);
        lv_obj_add_flag(multiplayer_quadrants[i], LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(multiplayer_quadrants[i], event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(multiplayer_quadrants[i], event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);

        label_multiplayer_name[i] = lv_label_create(multiplayer_quadrants[i]);
        lv_label_set_text(label_multiplayer_name[i], player_names[i]);
        lv_obj_set_style_text_color(label_multiplayer_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_multiplayer_name[i], &lv_font_montserrat_14, 0);
        lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, 0, 30);

        label_multiplayer_life[i] = lv_label_create(multiplayer_quadrants[i]);
        lv_label_set_text(label_multiplayer_life[i], "40");
        lv_obj_set_style_text_color(label_multiplayer_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_multiplayer_life[i], &lv_font_montserrat_32, 0);
        lv_obj_align(label_multiplayer_life[i], LV_ALIGN_CENTER, 0, -30);

    }

    refresh_multiplayer_ui();
}

void build_multiplayer_menu_screen(void)
{
    quad_item_t items[4] = {
        {"Rename",      event_multiplayer_menu_rename,     true,  LV_EVENT_CLICKED},
        {"Cmd\nDamage", event_multiplayer_menu_cmd_damage, true,  LV_EVENT_CLICKED},
        {"All\nDamage", event_multiplayer_menu_all_damage, true,  LV_EVENT_CLICKED},
        {"",            NULL,                              false, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_player_menu, items);

    /* Long-press Rename to rename all players sequentially */
    lv_obj_t *rename_btn = lv_obj_get_child(screen_player_menu, 0);
    lv_obj_add_event_cb(rename_btn, event_multiplayer_menu_rename_all, LV_EVENT_LONG_PRESSED, NULL);
}

void build_multiplayer_all_damage_screen(void)
{
    screen_player_all_damage = lv_obj_create(NULL);
    lv_obj_set_size(screen_player_all_damage, 360, 360);
    lv_obj_set_style_bg_color(screen_player_all_damage, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_player_all_damage, 0, 0);
    lv_obj_set_scrollbar_mode(screen_player_all_damage, LV_SCROLLBAR_MODE_OFF);

    label_multiplayer_all_damage_title = lv_label_create(screen_player_all_damage);
    lv_obj_set_style_text_color(label_multiplayer_all_damage_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_multiplayer_all_damage_title, LV_ALIGN_TOP_MID, 0, 26);

    label_multiplayer_all_damage_value = lv_label_create(screen_player_all_damage);
    lv_obj_set_style_text_color(label_multiplayer_all_damage_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_multiplayer_all_damage_value, LV_ALIGN_CENTER, 0, -8);

    label_multiplayer_all_damage_hint = lv_label_create(screen_player_all_damage);
    lv_label_set_text(label_multiplayer_all_damage_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_multiplayer_all_damage_hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_multiplayer_all_damage_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_multiplayer_all_damage_hint, LV_ALIGN_CENTER, 0, 38);

    lv_obj_t *btn = make_button(screen_player_all_damage, "apply", 120, 46, event_multiplayer_all_damage_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

// ---------- 2-player screen (top/bottom split) ----------
void build_multiplayer_2p_screen(void)
{
    /* Panel 0 = top = P2, Panel 1 = bottom = P1 */
    static const lv_coord_t panel_y[2] = {0, 182};
    static const int panel_player[2] = {1, 0};
    int i;

    screen_2p = lv_obj_create(NULL);
    lv_obj_set_size(screen_2p, 360, 360);
    lv_obj_set_style_bg_color(screen_2p, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_2p, 0, 0);
    lv_obj_set_scrollbar_mode(screen_2p, LV_SCROLLBAR_MODE_OFF);

    for (i = 0; i < 2; i++) {
        int p = panel_player[i];
        mp2_panels[i] = lv_btn_create(screen_2p);
        lv_obj_remove_style_all(mp2_panels[i]);
        lv_obj_set_style_bg_opa(mp2_panels[i], LV_OPA_COVER, 0);
        lv_obj_set_size(mp2_panels[i], 360, 178);
        lv_obj_set_pos(mp2_panels[i], 0, panel_y[i]);
        lv_obj_set_style_radius(mp2_panels[i], 0, 0);
        lv_obj_set_style_border_width(mp2_panels[i], 1, 0);
        lv_obj_set_style_border_color(mp2_panels[i], lv_color_black(), 0);
        lv_obj_set_style_shadow_width(mp2_panels[i], 0, 0);
        lv_obj_add_event_cb(mp2_panels[i], event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)p);
        lv_obj_add_event_cb(mp2_panels[i], event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)p);

        label_mp2_name[i] = lv_label_create(mp2_panels[i]);
        lv_label_set_text(label_mp2_name[i], multiplayer_names[p]);
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
    /* 3p shares screen_4p with 4p; nothing to build */
    screen_3p = screen_4p;
}
