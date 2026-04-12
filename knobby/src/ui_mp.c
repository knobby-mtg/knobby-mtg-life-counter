#include "ui_mp.h"
#include "ui_player_menu.h"
#include "ui_1p.h"
#include "game.h"
#include "storage.h"

// ---------- screens ----------
lv_obj_t *screen_4p = NULL;
lv_obj_t *screen_2p = NULL;
lv_obj_t *screen_3p = NULL;

// ---------- widgets ----------
static lv_obj_t *multiplayer_quadrants[MULTIPLAYER_COUNT];
static lv_obj_t *label_player_life[MULTIPLAYER_COUNT];
static lv_obj_t *label_multiplayer_name[MULTIPLAYER_COUNT];
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

static const lv_font_t *get_counter_badge_font(const counter_definition_t *definition)
{
    if (definition != NULL && definition->icon_text != NULL) {
        return &mana_counter_icons_16;
    }

    return &lv_font_montserrat_14;
}

static const char *get_counter_badge_text(const counter_definition_t *definition)
{
    if (definition == NULL) return "?";
    if (definition->icon_text != NULL) return definition->icon_text;
    if (definition->badge_text != NULL) return definition->badge_text;
    return "?";
}

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
                               lv_obj_t **row_out, lv_obj_t **value_out, int player_index)
{
    const counter_definition_t *definition = get_counter_definition(type);
    lv_obj_t *row;
    lv_obj_t *glyph;

    row = make_plain_box(parent, 34, 34);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

    glyph = lv_label_create(row);
    lv_label_set_text(glyph, get_counter_badge_text(definition));
    lv_obj_set_style_text_color(glyph, get_player_text_color(player_index), 0);
    lv_obj_set_style_text_font(glyph,
        (type == COUNTER_TYPE_POISON) ? &mana_poison_icon_bold_16
                                      : get_counter_badge_font(definition), 0);
    lv_obj_align(glyph, LV_ALIGN_TOP_MID, 0, 0);

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
            get_counter_value(player_index, (counter_type_t)type) <= 0) {
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

        value = get_counter_value(player_index, (counter_type_t)counter_type);

        snprintf(buf, sizeof(buf), "%d", value);
        lv_label_set_text(value_labels[counter_type], buf);
        lv_obj_set_style_text_color(value_labels[counter_type], text_color, 0);
        /* Also update the icon/glyph color (first child of the row) */
        {
            lv_obj_t *glyph = lv_obj_get_child(rows[counter_type], 0);
            if (glyph != NULL) {
                lv_obj_set_style_text_color(glyph, text_color, 0);
            }
        }
        lv_obj_clear_flag(rows[counter_type], LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(rows[counter_type], LV_ALIGN_CENTER, local_x, local_y);
        apply_object_rotation(rows[counter_type], row_angle, 0, 0);
    }
}

static lv_color_t refresh_mp_panel(lv_obj_t *panel, lv_obj_t *life_lbl, lv_obj_t *name_lbl, int i, int color_i)
{
    char buf[8];
    bool preview_here = life_preview_active && (preview_player == i);
    bool selected = (i == selected_player);
    lv_color_t bg_color;
    lv_color_t text_color;

    {
        int vib;
        if (selected_player < 0) vib = LIFE_VIB_MID;
        else vib = selected ? LIFE_VIB_VIV : LIFE_VIB_DIM;
        bg_color = get_effective_player_color(i, color_i, vib);
        text_color = color_is_light(bg_color) ? lv_color_black() : lv_color_white();
    }

    if (panel != NULL) {
        lv_obj_set_style_bg_color(panel, bg_color, 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    }

    if (player_eliminated[i]) {
        bg_color = lv_color_hex(0x404040);
        text_color = lv_color_hex(0x808080);
        if (panel != NULL) {
            lv_obj_set_style_bg_color(panel, bg_color, 0);
        }
    }

    if (life_lbl != NULL) {
        if (preview_here) {
            snprintf(buf, sizeof(buf), "%+d", pending_life_delta);
            lv_label_set_text(life_lbl, buf);
            {
                lv_color_t preview_c;
                if (nvs_get_color_mode() == COLOR_MODE_PLAYER && !player_has_override[i]) {
                    preview_c = get_player_preview_color(color_i, pending_life_delta);
                    if (color_is_light(bg_color) && color_is_light(preview_c))
                        preview_c = lv_color_black();
                    else if (!color_is_light(bg_color) && !color_is_light(preview_c))
                        preview_c = lv_color_white();
                } else {
                    /* Life mode, or player has override: pick purely on bg contrast */
                    preview_c = color_is_light(bg_color) ? lv_color_black() : lv_color_white();
                }
                lv_obj_set_style_text_color(life_lbl, preview_c, 0);
            }
        } else {
            snprintf(buf, sizeof(buf), "%d", player_life[i]);
            lv_label_set_text(life_lbl, buf);
            lv_obj_set_style_text_color(life_lbl, text_color, 0);
        }
    }

    if (name_lbl != NULL) {
        if (preview_here) {
            char total_buf[16];
            int new_total = player_life[i] + pending_life_delta;
            snprintf(total_buf, sizeof(total_buf), "= %d", new_total);
            lv_label_set_text(name_lbl, total_buf);
        } else {
            lv_label_set_text(name_lbl, player_names[i]);
        }
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

    /* X nudge toward screen center per quadrant (left panels +, right panels -) */
    static const lv_coord_t center_nudge_x[MULTIPLAYER_COUNT] = {10, 10, -10, -10};

    for (i = 0; i < MULTIPLAYER_COUNT; i++) {
        lv_coord_t nx = (orientation_mode != ORIENTATION_MODE_CENTRIC) ? center_nudge_x[i] : 0;
        angle = get_4p_orientation_angle(orientation_mode, i);
        counter_angle = get_counter_row_angle(orientation_mode, multiplayer_quadrants[i], angle);
        text_color = refresh_mp_panel(multiplayer_quadrants[i], label_player_life[i], label_multiplayer_name[i], i, i);
        if (label_player_life[i] != NULL) {
            lv_obj_clear_flag(label_player_life[i], LV_OBJ_FLAG_HIDDEN);
            if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
                lv_obj_set_style_text_font(label_player_life[i], &lv_font_montserrat_bold_44, 0);
            } else {
                lv_obj_set_style_text_font(label_player_life[i], &lv_font_montserrat_bold_56, 0);
            }
            if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
                lv_obj_align(label_player_life[i], LV_ALIGN_CENTER, nx, -10);
            } else {
                lv_obj_align(label_player_life[i], LV_ALIGN_CENTER, nx, -12);
            }
        }
        if (label_multiplayer_name[i] != NULL) {
            lv_obj_clear_flag(label_multiplayer_name[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, nx, 30);
        }
        if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
            apply_label_rotation(label_player_life[i], label_multiplayer_name[i],
                angle, 10, -30);
        } else {
            apply_label_rotation(label_player_life[i], label_multiplayer_name[i],
                angle, 12, -30);
        }
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

    /* X nudge: top-left panels right, top-right panels left, bottom panel centered */
    static const lv_coord_t nudge_3p_x[3] = {10, -10, 0};

    for (i = 0; i < 3; i++) {
        lv_coord_t nx = (orientation_mode != ORIENTATION_MODE_CENTRIC) ? nudge_3p_x[i] : 0;
        angle = get_3p_orientation_angle(orientation_mode, i);
        counter_angle = get_counter_row_angle(orientation_mode, mp3_panels[i], angle);
        text_color = refresh_mp_panel(mp3_panels[i], label_mp3_life[i], label_mp3_name[i],
                         panel_player[i], panel_player[i]);
        if (label_mp3_life[i] != NULL) {
            if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
                lv_obj_set_style_text_font(label_mp3_life[i], &lv_font_montserrat_bold_44, 0);
                lv_obj_align(label_mp3_life[i], LV_ALIGN_CENTER, nx, -10);
            } else {
                lv_obj_set_style_text_font(label_mp3_life[i], &lv_font_montserrat_bold_56, 0);
                lv_obj_align(label_mp3_life[i], LV_ALIGN_CENTER, nx, -12);
            }
        }
        if (label_mp3_name[i] != NULL)
            lv_obj_align(label_mp3_name[i], LV_ALIGN_CENTER, nx, 30);
        if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
            apply_label_rotation(label_mp3_life[i], label_mp3_name[i],
                angle, 10, -30);
        } else {
            apply_label_rotation(label_mp3_life[i], label_mp3_name[i],
                angle, 12, -30);
        }
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

// ---------- navigation ----------
void open_multiplayer_screen(void)
{
    int track = nvs_get_players_to_track();
    refresh_multiplayer_ui();
    if (track == 2) load_screen_if_needed(screen_2p);
    else if (track == 3) load_screen_if_needed(screen_3p);
    else load_screen_if_needed(screen_4p);
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
    selected_player = -1;
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
    if (selected_player >= 0 && ms > 0) {
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
    if (player_eliminated[player]) return;

    if (life_preview_active && preview_player != player) {
        life_preview_commit_cb(NULL);
    }

    if (selected_player == player) {
        /* Deselecting the same player: if there's an active preview for
         * this player, commit it immediately so the altered life total is
         * shown (same behavior as selecting another player). */
        if (life_preview_active && preview_player == player) {
            life_preview_commit_cb(NULL);
        }
        selected_player = -1;
    } else {
        selected_player = player;
    }
    select_kick_timer();
    refresh_multiplayer_ui();
}

static void event_multiplayer_open_menu(lv_event_t *e)
{
    int quad = (int)(intptr_t)lv_event_get_user_data(e);
    int player = quad_to_player(quad);

    if (player < 0) return;
    if (player_eliminated[player]) {
        menu_player = player;
        load_screen_if_needed(screen_eliminated_player_menu);
        return;
    }

    if (life_preview_active && preview_player != player) {
        life_preview_commit_cb(NULL);
    }

    selected_player = player;
    refresh_multiplayer_ui();
    open_player_menu(selected_player);
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
        lv_obj_set_style_text_font(label_multiplayer_name[i], &lv_font_montserrat_22, 0);
        lv_obj_align(label_multiplayer_name[i], LV_ALIGN_CENTER, 0, 30);

        label_player_life[i] = lv_label_create(multiplayer_quadrants[i]);
        lv_label_set_text(label_player_life[i], "40");
        lv_obj_set_style_text_color(label_player_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_player_life[i], &lv_font_montserrat_bold_56, 0);
        lv_obj_align(label_player_life[i], LV_ALIGN_CENTER, 0, -30);

        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_4p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_4p[i][COUNTER_TYPE_COMMANDER_TAX], i);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_4p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_4p[i][COUNTER_TYPE_PARTNER_TAX], i);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_POISON,
            &counter_row_4p[i][COUNTER_TYPE_POISON],
            &counter_value_4p[i][COUNTER_TYPE_POISON], i);
        create_counter_row(multiplayer_quadrants[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_4p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_4p[i][COUNTER_TYPE_EXPERIENCE], i);

    }

    refresh_multiplayer_ui();
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
        lv_label_set_text(label_mp2_name[i], player_names[p]);
        lv_obj_set_style_text_color(label_mp2_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp2_name[i], &lv_font_montserrat_22, 0);
        lv_obj_align(label_mp2_name[i], LV_ALIGN_CENTER, 0, 30);

        label_mp2_life[i] = lv_label_create(mp2_panels[i]);
        lv_label_set_text(label_mp2_life[i], "40");
        lv_obj_set_style_text_color(label_mp2_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp2_life[i], &lv_font_montserrat_bold_56, 0);
        lv_obj_align(label_mp2_life[i], LV_ALIGN_CENTER, 0, -10);

        create_counter_row(mp2_panels[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_2p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_2p[i][COUNTER_TYPE_COMMANDER_TAX], p);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_2p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_2p[i][COUNTER_TYPE_PARTNER_TAX], p);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_POISON,
            &counter_row_2p[i][COUNTER_TYPE_POISON],
            &counter_value_2p[i][COUNTER_TYPE_POISON], p);
        create_counter_row(mp2_panels[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_2p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_2p[i][COUNTER_TYPE_EXPERIENCE], p);

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
        lv_label_set_text(label_mp3_name[i], player_names[p]);
        lv_obj_set_style_text_color(label_mp3_name[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp3_name[i], &lv_font_montserrat_22, 0);
        lv_obj_align(label_mp3_name[i], LV_ALIGN_CENTER, 0, 30);

        label_mp3_life[i] = lv_label_create(mp3_panels[i]);
        lv_label_set_text(label_mp3_life[i], "40");
        lv_obj_set_style_text_color(label_mp3_life[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_mp3_life[i], &lv_font_montserrat_bold_56, 0);
        lv_obj_align(label_mp3_life[i], LV_ALIGN_CENTER, 0, -12);

        create_counter_row(mp3_panels[i], COUNTER_TYPE_COMMANDER_TAX,
            &counter_row_3p[i][COUNTER_TYPE_COMMANDER_TAX],
            &counter_value_3p[i][COUNTER_TYPE_COMMANDER_TAX], p);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_PARTNER_TAX,
            &counter_row_3p[i][COUNTER_TYPE_PARTNER_TAX],
            &counter_value_3p[i][COUNTER_TYPE_PARTNER_TAX], p);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_POISON,
            &counter_row_3p[i][COUNTER_TYPE_POISON],
            &counter_value_3p[i][COUNTER_TYPE_POISON], p);
        create_counter_row(mp3_panels[i], COUNTER_TYPE_EXPERIENCE,
            &counter_row_3p[i][COUNTER_TYPE_EXPERIENCE],
            &counter_value_3p[i][COUNTER_TYPE_EXPERIENCE], p);
    }
}

