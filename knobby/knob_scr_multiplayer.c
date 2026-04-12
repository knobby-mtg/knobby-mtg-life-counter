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
lv_obj_t *screen_player_counters_menu = NULL;
lv_obj_t *screen_player_counter_edit = NULL;

// ---------- widgets ----------
static lv_obj_t *multiplayer_quadrants[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_life[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_name[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_all_damage_title = NULL;
static lv_obj_t *label_multiplayer_all_damage_value = NULL;
static lv_obj_t *label_multiplayer_all_damage_hint = NULL;
static lv_obj_t *label_multiplayer_counter_edit_title = NULL;
static lv_obj_t *label_multiplayer_counter_edit_value = NULL;
static lv_obj_t *label_multiplayer_counter_edit_hint = NULL;
static lv_obj_t *counter_row_4p[MULTIPLAYER_COUNT][COUNTER_TYPE_COUNT];
static lv_obj_t *counter_value_4p[MULTIPLAYER_COUNT][COUNTER_TYPE_COUNT];

// ---------- 2-player widgets ----------
static lv_obj_t *mp2_panels[2];
static lv_obj_t *label_mp2_life[2];
static lv_obj_t *label_mp2_name[2];
static lv_obj_t *counter_row_2p[2][COUNTER_TYPE_COUNT];
static lv_obj_t *counter_value_2p[2][COUNTER_TYPE_COUNT];

// ---------- selection auto-deselect ----------
static lv_timer_t *select_timeout_timer = NULL;

// ---------- 3-player widgets ----------
static lv_obj_t *mp3_panels[3];
static lv_obj_t *label_mp3_life[3];
static lv_obj_t *label_mp3_name[3];
static lv_obj_t *counter_row_3p[3][COUNTER_TYPE_COUNT];
static lv_obj_t *counter_value_3p[3][COUNTER_TYPE_COUNT];

// ---------- forward declarations ----------
static void open_multiplayer_all_damage_screen(void);
static void open_multiplayer_counter_edit_screen(counter_type_t type);

static void apply_object_rotation(lv_obj_t *obj, int16_t angle, int pivot_x, int pivot_y)
{
    if (obj == NULL) return;

    lv_obj_set_style_transform_angle(obj, angle, 0);
    if (angle != 0) {
        lv_obj_update_layout(obj);
        lv_obj_set_style_transform_pivot_x(obj,
            lv_obj_get_width(obj) / 2 + pivot_x, 0);
        lv_obj_set_style_transform_pivot_y(obj,
            lv_obj_get_height(obj) / 2 + pivot_y, 0);
    }
}

static void create_counter_row(lv_obj_t *parent, counter_type_t type,
                               lv_obj_t **row_out, lv_obj_t **value_out)
{
    const counter_definition_t *definition = get_counter_definition(type);
    lv_obj_t *row;
    lv_obj_t *icon;
    lv_obj_t *glyph;

    row = make_plain_box(parent, 34, 34);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

    icon = lv_obj_create(row);
    lv_obj_remove_style_all(icon);
    lv_obj_set_size(icon, 16, 16);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(icon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(icon,
        definition != NULL ? lv_color_hex(definition->accent_color) : lv_color_hex(0x303030), 0);

    glyph = lv_label_create(icon);
    lv_label_set_text(glyph, definition != NULL ? definition->badge_text : "?");
    lv_obj_set_style_text_color(glyph, lv_color_white(), 0);
    lv_obj_set_style_text_font(glyph, &lv_font_montserrat_14, 0);
    lv_obj_center(glyph);

    *value_out = lv_label_create(row);
    lv_label_set_text(*value_out, "0");
    lv_obj_set_style_text_color(*value_out, lv_color_white(), 0);
    lv_obj_set_style_text_font(*value_out, &lv_font_montserrat_14, 0);
    lv_obj_align(*value_out, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(*value_out, LV_TEXT_ALIGN_CENTER, 0);

    *row_out = row;
}

// ---------- rotation helper ----------
static void apply_label_rotation(lv_obj_t *life_lbl, lv_obj_t *name_lbl,
                                  int16_t angle, int life_pivot_y, int name_pivot_y)
{
    apply_object_rotation(life_lbl, angle, 0, life_pivot_y);
    apply_object_rotation(name_lbl, angle, 0, name_pivot_y);
}

static void get_counter_equator_anchor(lv_obj_t *panel,
                                       lv_coord_t *anchor_x, lv_coord_t *anchor_y)
{
    lv_obj_t *parent;
    lv_coord_t panel_y;
    lv_coord_t panel_h;
    lv_coord_t parent_h;
    lv_coord_t panel_center_y;
    lv_coord_t target_world_y;
    const lv_coord_t equator_gap = 24;
    const lv_coord_t edge_margin = 24;

    if (anchor_x == NULL || anchor_y == NULL) return;

    *anchor_x = 0;
    *anchor_y = 0;
    if (panel == NULL) return;

    parent = lv_obj_get_parent(panel);
    if (parent == NULL) return;

    panel_y = lv_obj_get_y(panel);
    panel_h = lv_obj_get_height(panel);
    parent_h = lv_obj_get_height(parent);
    panel_center_y = panel_y + (panel_h / 2);

    target_world_y = (parent_h / 2) + ((panel_center_y < (parent_h / 2)) ? -equator_gap : equator_gap);
    if (target_world_y < panel_y + edge_margin) target_world_y = panel_y + edge_margin;
    if (target_world_y > panel_y + panel_h - edge_margin) target_world_y = panel_y + panel_h - edge_margin;

    *anchor_x = 0;
    *anchor_y = target_world_y - panel_center_y;
}

static int16_t get_4p_orientation_angle(int mode, int panel_index)
{
    static const int16_t angled_rot[MULTIPLAYER_COUNT] = {450, 1350, 2250, 3150};

    switch (mode) {
        case ORIENTATION_MODE_CENTRIC:
            return angled_rot[panel_index];
        case ORIENTATION_MODE_TABLETOP:
            return (panel_index == 1 || panel_index == 2) ? 1800 : 0;
        default:
            return 0;
    }
}

static int16_t get_3p_orientation_angle(int mode, int panel_index)
{
    static const int16_t angled_rot[3] = {1350, 2250, 0};

    switch (mode) {
        case ORIENTATION_MODE_CENTRIC:
            return angled_rot[panel_index];
        case ORIENTATION_MODE_TABLETOP:
            return (panel_index < 2) ? 1800 : 0;
        default:
            return 0;
    }
}

static int16_t get_counter_row_angle(int orientation_mode, lv_obj_t *panel, int16_t panel_angle)
{
    if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
        lv_obj_t *parent;

        if (panel == NULL) return 0;

        parent = lv_obj_get_parent(panel);
        if (parent == NULL) return 0;

        if ((lv_obj_get_y(panel) + (lv_obj_get_height(panel) / 2)) < (lv_obj_get_height(parent) / 2)) {
            return 1800;
        }

        return 0;
    }

    return panel_angle;
}

// ---------- refresh helpers ----------
static void refresh_counter_rows(lv_obj_t *panel, lv_obj_t **rows, lv_obj_t **value_labels,
                                 int player_index, lv_color_t text_color,
                                 int16_t panel_angle, int16_t row_angle)
{
    int type;
    int visible_count = 0;
    int visible_types[COUNTER_TYPE_COUNT];
    char buf[8];
    const lv_coord_t step = 30;
    lv_coord_t anchor_x = 0;
    lv_coord_t anchor_y = 0;

    (void)panel_angle;
    get_counter_equator_anchor(panel, &anchor_x, &anchor_y);

    for (type = 0; type < COUNTER_TYPE_COUNT; type++) {
        if (rows[type] == NULL || value_labels[type] == NULL) continue;

        if (!counter_type_is_enabled((counter_type_t)type) ||
            get_multiplayer_counter_value(player_index, (counter_type_t)type) <= 0) {
            lv_obj_add_flag(rows[type], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        visible_types[visible_count] = type;
        visible_count++;
    }

    for (type = 0; type < visible_count; type++) {
        int value;
        int counter_type = visible_types[type];
        lv_coord_t x_offset = (lv_coord_t)((type * step) - ((visible_count - 1) * step / 2));
        lv_coord_t local_x = anchor_x + x_offset;
        lv_coord_t local_y = anchor_y;

        value = get_multiplayer_counter_value(player_index, (counter_type_t)counter_type);

        snprintf(buf, sizeof(buf), "%d", value);
        lv_label_set_text(value_labels[counter_type], buf);
        lv_obj_set_style_text_color(value_labels[counter_type], text_color, 0);
        lv_obj_clear_flag(rows[counter_type], LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(rows[counter_type], LV_ALIGN_CENTER, local_x, local_y);
        apply_object_rotation(rows[counter_type], row_angle, 0, 0);
    }
}

static lv_color_t refresh_mp_panel(lv_obj_t *panel, lv_obj_t *life_lbl, lv_obj_t *name_lbl, int i, int color_i)
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

    return text_color;
}

static void refresh_multiplayer_4p_ui(void)
{
    int orientation_mode = nvs_get_orientation();
    int i;
    int16_t angle;
    int16_t counter_angle;
    lv_color_t text_color;

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        angle = get_4p_orientation_angle(orientation_mode, i);
        counter_angle = get_counter_row_angle(orientation_mode, multiplayer_quadrants[i], angle);
        text_color = refresh_mp_panel(multiplayer_quadrants[i], label_multiplayer_life[i], label_multiplayer_name[i], i, i);
        if (label_multiplayer_life[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_life[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_life[i], LV_ALIGN_CENTER, 0, angle != 0 ? -30 : -12);
        }
        if (label_multiplayer_name[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_name[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, 0, 30);
        }
        apply_label_rotation(label_multiplayer_life[i], label_multiplayer_name[i],
            angle, 30, -30);
        refresh_counter_rows(multiplayer_quadrants[i], counter_row_4p[i], counter_value_4p[i], i, text_color, angle, counter_angle);
    }
}

static void refresh_multiplayer_2p_ui(void)
{
    /* Panel 0 = top = P2 (player 1), Panel 1 = bottom = P1 (player 0) */
    static const int panel_player[2] = {1, 0};
    static const int panel_color[2]  = {0, 1};
    int orientation_mode = nvs_get_orientation();
    int i;
    int16_t angle;
    lv_color_t text_color;
    for (i = 0; i < 2; i++) {
        angle = (i == 0 && orientation_mode != ORIENTATION_MODE_ABSOLUTE) ? 1800 : 0;
        text_color = refresh_mp_panel(mp2_panels[i], label_mp2_life[i], label_mp2_name[i],
                         panel_player[i], panel_color[i]);
        apply_label_rotation(label_mp2_life[i], label_mp2_name[i],
            angle, 10, -30);
        refresh_counter_rows(mp2_panels[i], counter_row_2p[i], counter_value_2p[i], panel_player[i],
            text_color, angle, get_counter_row_angle(orientation_mode, mp2_panels[i], angle));
    }
}

static void refresh_multiplayer_3p_ui(void)
{
    /* Panel 0 = top-left = P2 (player 1), Panel 1 = top-right = P3 (player 2), Panel 2 = bottom = P1 (player 0) */
    static const int panel_player[3] = {1, 2, 0};
    int orientation_mode = nvs_get_orientation();
    int i;
    int16_t angle;
    int16_t counter_angle;
    lv_color_t text_color;

    for (i = 0; i < 3; i++) {
        angle = get_3p_orientation_angle(orientation_mode, i);
        counter_angle = get_counter_row_angle(orientation_mode, mp3_panels[i], angle);
        text_color = refresh_mp_panel(mp3_panels[i], label_mp3_life[i], label_mp3_name[i],
                         panel_player[i], panel_player[i]);
        if (label_mp3_life[i] != NULL)
            lv_obj_align(label_mp3_life[i], LV_ALIGN_CENTER, 0, angle != 0 ? -30 : -12);
        if (label_mp3_name[i] != NULL)
            lv_obj_align(label_mp3_name[i], LV_ALIGN_CENTER, 0, 30);
        apply_label_rotation(label_mp3_life[i], label_mp3_name[i],
            angle, 30, -30);
        refresh_counter_rows(mp3_panels[i], counter_row_3p[i], counter_value_3p[i], panel_player[i], text_color, angle, counter_angle);
    }
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

void refresh_multiplayer_counter_edit_ui(void)
{
    char title_buf[48];
    char value_buf[16];
    const counter_definition_t *definition = get_counter_definition(multiplayer_counter_edit_type);

    if (definition == NULL) return;

    if (label_multiplayer_counter_edit_title != NULL) {
        snprintf(title_buf, sizeof(title_buf), "%s\n%s",
            multiplayer_names[multiplayer_menu_player], definition->display_name);
        lv_label_set_text(label_multiplayer_counter_edit_title, title_buf);
    }

    if (label_multiplayer_counter_edit_value != NULL) {
        snprintf(value_buf, sizeof(value_buf), "%d", multiplayer_counter_edit_value);
        lv_label_set_text(label_multiplayer_counter_edit_value, value_buf);
    }
}

// ---------- navigation ----------
void open_multiplayer_screen(void)
{
    int track = nvs_get_players_to_track();
    refresh_multiplayer_ui();
    if (track == 2) load_screen_if_needed(screen_2p);
    else if (track == 3) load_screen_if_needed(screen_3p);
    else load_screen_if_needed(screen_4p);
}

void open_multiplayer_menu_screen(int player_index)
{
    multiplayer_menu_player = player_index;
    load_screen_if_needed(screen_player_menu);
}

void open_multiplayer_counter_menu_screen(void)
{
    load_screen_if_needed(screen_player_counters_menu);
}

static void open_multiplayer_all_damage_screen(void)
{
    multiplayer_all_damage_value = 0;
    refresh_multiplayer_all_damage_ui();
    load_screen_if_needed(screen_player_all_damage);
}

static void open_multiplayer_counter_edit_screen(counter_type_t type)
{
    begin_multiplayer_counter_edit(multiplayer_menu_player, type);
    refresh_multiplayer_counter_edit_ui();
    load_screen_if_needed(screen_player_counter_edit);
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
        /* Deselecting the same player: if there's an active preview for
         * this player, commit it immediately so the altered life total is
         * shown (same behavior as selecting another player). */
        if (multiplayer_life_preview_active && multiplayer_preview_player == player) {
            multiplayer_life_preview_commit_cb(NULL);
        }
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

static void event_multiplayer_menu_counters(lv_event_t *e)
{
    (void)e;
    open_multiplayer_counter_menu_screen();
}

static void event_multiplayer_counter_commander_tax(lv_event_t *e)
{
    (void)e;
    open_multiplayer_counter_edit_screen(COUNTER_TYPE_COMMANDER_TAX);
}

static void event_multiplayer_counter_partner_tax(lv_event_t *e)
{
    (void)e;
    open_multiplayer_counter_edit_screen(COUNTER_TYPE_PARTNER_TAX);
}

static void event_multiplayer_counter_poison(lv_event_t *e)
{
    (void)e;
    open_multiplayer_counter_edit_screen(COUNTER_TYPE_POISON);
}

static void event_multiplayer_counter_experience(lv_event_t *e)
{
    (void)e;
    open_multiplayer_counter_edit_screen(COUNTER_TYPE_EXPERIENCE);
}

static void event_multiplayer_all_damage_apply(lv_event_t *e)
{
    int i;

    (void)e;
    for (i = 0; i < nvs_get_players_to_track(); i++) {
        damage_log_add(i, -multiplayer_all_damage_value, LOG_EVT_LIFE, -1);
        multiplayer_life[i] = clamp_life(multiplayer_life[i] - multiplayer_all_damage_value);
    }

    refresh_multiplayer_ui();
    open_multiplayer_screen();
}

static void event_multiplayer_counter_apply(lv_event_t *e)
{
    (void)e;
    apply_multiplayer_counter_edit();
    refresh_multiplayer_counter_edit_ui();
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

        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_4p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_4p[i][COUNTER_TYPE_COMMANDER_TAX]);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_4p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_4p[i][COUNTER_TYPE_PARTNER_TAX]);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_POISON,
            &counter_row_4p[i][COUNTER_TYPE_POISON],
            &counter_value_4p[i][COUNTER_TYPE_POISON]);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_4p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_4p[i][COUNTER_TYPE_EXPERIENCE]);

    }

    refresh_multiplayer_ui();
}

void build_multiplayer_menu_screen(void)
{
    quad_item_t items[4] = {
        {"Rename",      event_multiplayer_menu_rename,     true,  LV_EVENT_CLICKED},
        {"Commander\nDamage", event_multiplayer_menu_cmd_damage, true,  LV_EVENT_CLICKED},
        {"All\nDamage", event_multiplayer_menu_all_damage, true,  LV_EVENT_CLICKED},
        {"Counters",    event_multiplayer_menu_counters,   true,  LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_player_menu, items);

    /* Long-press Rename to rename all players sequentially */
    lv_obj_t *rename_btn = lv_obj_get_child(screen_player_menu, 0);
    lv_obj_add_event_cb(rename_btn, event_multiplayer_menu_rename_all, LV_EVENT_LONG_PRESSED, NULL);
}

void build_multiplayer_counter_menu_screen(void)
{
    quad_item_t items[4] = {
        {"Commander\nTax", event_multiplayer_counter_commander_tax, true, LV_EVENT_CLICKED},
        {"Partner\nTax", event_multiplayer_counter_partner_tax, true, LV_EVENT_CLICKED},
        {"Poison", event_multiplayer_counter_poison, true, LV_EVENT_CLICKED},
        {"Experience", event_multiplayer_counter_experience, true, LV_EVENT_CLICKED},
    };

    build_quad_screen(&screen_player_counters_menu, items);
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

    lv_obj_t *btn = make_button(screen_player_all_damage, "Apply", 120, 46, event_multiplayer_all_damage_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

void build_multiplayer_counter_edit_screen(void)
{
    lv_obj_t *btn;

    screen_player_counter_edit = lv_obj_create(NULL);
    lv_obj_set_size(screen_player_counter_edit, 360, 360);
    lv_obj_set_style_bg_color(screen_player_counter_edit, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_player_counter_edit, 0, 0);
    lv_obj_set_scrollbar_mode(screen_player_counter_edit, LV_SCROLLBAR_MODE_OFF);

    label_multiplayer_counter_edit_title = lv_label_create(screen_player_counter_edit);
    lv_label_set_text(label_multiplayer_counter_edit_title, "P1\nCommander Tax");
    lv_obj_set_style_text_color(label_multiplayer_counter_edit_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_counter_edit_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(label_multiplayer_counter_edit_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_multiplayer_counter_edit_title, LV_ALIGN_TOP_MID, 0, 42);

    label_multiplayer_counter_edit_value = lv_label_create(screen_player_counter_edit);
    lv_label_set_text(label_multiplayer_counter_edit_value, "0");
    lv_obj_set_style_text_color(label_multiplayer_counter_edit_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_multiplayer_counter_edit_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_multiplayer_counter_edit_value, LV_ALIGN_CENTER, 0, -4);

    label_multiplayer_counter_edit_hint = lv_label_create(screen_player_counter_edit);
    lv_label_set_text(label_multiplayer_counter_edit_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_multiplayer_counter_edit_hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_multiplayer_counter_edit_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_multiplayer_counter_edit_hint, LV_ALIGN_CENTER, 0, 34);

    btn = make_button(screen_player_counter_edit, "Apply", 120, 46, event_multiplayer_counter_apply);
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

        create_counter_row(mp2_panels[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_2p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_2p[i][COUNTER_TYPE_COMMANDER_TAX]);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_2p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_2p[i][COUNTER_TYPE_PARTNER_TAX]);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_POISON,
            &counter_row_2p[i][COUNTER_TYPE_POISON],
            &counter_value_2p[i][COUNTER_TYPE_POISON]);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_2p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_2p[i][COUNTER_TYPE_EXPERIENCE]);

    }
}

// ---------- 3-player screen (two top quadrants + one full-width bottom panel) ----------
void build_multiplayer_3p_screen(void)
{
    /* Panel 0 = top-left = P2, Panel 1 = top-right = P3, Panel 2 = bottom = P1 */
    static const lv_coord_t panel_x[3] = {0,   180, 0};
    static const lv_coord_t panel_y[3] = {0,   0,   180};
    static const lv_coord_t panel_w[3] = {180, 180, 360};
    static const lv_coord_t panel_h[3] = {180, 180, 180};
    static const int panel_player[3]   = {1,   2,   0};
    int i;

    screen_3p = lv_obj_create(NULL);
    lv_obj_set_size(screen_3p, 360, 360);
    lv_obj_set_style_bg_color(screen_3p, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_3p, 0, 0);
    lv_obj_set_scrollbar_mode(screen_3p, LV_SCROLLBAR_MODE_OFF);

    for (i = 0; i < 3; i++) {
        int p = panel_player[i];
        mp3_panels[i] = lv_btn_create(screen_3p);
        lv_obj_remove_style_all(mp3_panels[i]);
        lv_obj_set_style_bg_opa(mp3_panels[i], LV_OPA_COVER, 0);
        lv_obj_set_size(mp3_panels[i], panel_w[i], panel_h[i]);
        lv_obj_set_pos(mp3_panels[i], panel_x[i], panel_y[i]);
        lv_obj_set_style_radius(mp3_panels[i], 0, 0);
        lv_obj_set_style_border_width(mp3_panels[i], 1, 0);
        lv_obj_set_style_border_color(mp3_panels[i], lv_color_black(), 0);
        lv_obj_set_style_shadow_width(mp3_panels[i], 0, 0);
        lv_obj_add_event_cb(mp3_panels[i], event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)p);
        lv_obj_add_event_cb(mp3_panels[i], event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)p);

        label_mp3_name[i] = lv_label_create(mp3_panels[i]);
        lv_label_set_text(label_mp3_name[i], multiplayer_names[p]);
        lv_obj_set_style_text_color(label_mp3_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp3_name[i], &lv_font_montserrat_14, 0);
        lv_obj_align(label_mp3_name[i], LV_ALIGN_CENTER, 0, 30);

        label_mp3_life[i] = lv_label_create(mp3_panels[i]);
        lv_label_set_text(label_mp3_life[i], "40");
        lv_obj_set_style_text_color(label_mp3_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp3_life[i], &lv_font_montserrat_32, 0);
        lv_obj_align(label_mp3_life[i], LV_ALIGN_CENTER, 0, -12);

        create_counter_row(mp3_panels[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_3p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_3p[i][COUNTER_TYPE_COMMANDER_TAX]);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_3p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_3p[i][COUNTER_TYPE_PARTNER_TAX]);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_POISON,
            &counter_row_3p[i][COUNTER_TYPE_POISON],
            &counter_value_3p[i][COUNTER_TYPE_POISON]);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_3p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_3p[i][COUNTER_TYPE_EXPERIENCE]);
    }
}
