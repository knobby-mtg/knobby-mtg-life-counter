#include "ui_mp.h"
#include "ui_player_menu.h"
#include "ui_1p.h"
#include "game.h"
#include "storage.h"

#include <string.h>

// ---------- screens ----------
lv_obj_t *screen_multiplayer = NULL;

// ---------- layout specs ----------
typedef struct {
    lv_coord_t x, y, w, h;
    lv_coord_t nudge_x;     /* x offset for life/name labels in non-centric modes */
    int player_index;       /* which player this panel displays */
    int color_index;        /* color slot (differs from player only for 2p) */
} mp_panel_spec_t;

typedef struct {
    int panel_count;
    const mp_panel_spec_t *panels;
    int16_t (*angle_fn)(int orientation_mode, int panel_index);
    bool switch_font_by_orientation;
} mp_layout_spec_t;

/* ---------- shared widget state ---------- */
static struct {
    lv_obj_t *panels[MULTIPLAYER_COUNT];
    lv_obj_t *life_labels[MULTIPLAYER_COUNT];
    lv_obj_t *name_labels[MULTIPLAYER_COUNT];
    lv_obj_t *counter_rows[MULTIPLAYER_COUNT][COUNTER_TYPE_COUNT];
    lv_obj_t *counter_values[MULTIPLAYER_COUNT][COUNTER_TYPE_COUNT];
    const mp_layout_spec_t *layout;
} mp_state;

static lv_timer_t *select_timeout_timer = NULL;

/* ---------- small helpers ---------- */
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
    lv_obj_set_style_text_color(*value_out, get_player_text_color(player_index), 0);
    lv_obj_set_style_text_font(*value_out, &lv_font_montserrat_14, 0);
    lv_obj_align(*value_out, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(*value_out, LV_TEXT_ALIGN_CENTER, 0);

    *row_out = row;
}

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

/* ---------- per-mode angle functions ---------- */
static int16_t get_2p_orientation_angle(int mode, int panel_index)
{
    if (mode == ORIENTATION_MODE_ABSOLUTE) return 0;
    return (panel_index == 0) ? 1800 : 0;
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

/* ---------- per-mode panel specs ---------- */
/* 2p: top = P2 (player 1), bottom = P1 (player 0). Colors intentionally swapped. */
static const mp_panel_spec_t panels_2p[] = {
    {0,   0, 360, 178, 0, 1, 0},
    {0, 182, 360, 178, 0, 0, 1},
};

/* 3p: top-left = P2, top-right = P3, bottom = P1 */
static const mp_panel_spec_t panels_3p[] = {
    {0,   0, 180, 180,  10, 1, 1},
    {180, 0, 180, 180, -10, 2, 2},
    {0, 180, 360, 180,   0, 0, 0},
};

/* 4p: quadrants in order P1, P2, P3, P4 */
static const mp_panel_spec_t panels_4p[] = {
    {  0, 180, 180, 180,  10, 0, 0},
    {  0,   0, 180, 180,  10, 1, 1},
    {180,   0, 180, 180, -10, 2, 2},
    {180, 180, 180, 180, -10, 3, 3},
};

static const mp_layout_spec_t layout_2p = {
    .panel_count = 2,
    .panels = panels_2p,
    .angle_fn = get_2p_orientation_angle,
    .switch_font_by_orientation = false,
};

static const mp_layout_spec_t layout_3p = {
    .panel_count = 3,
    .panels = panels_3p,
    .angle_fn = get_3p_orientation_angle,
    .switch_font_by_orientation = true,
};

static const mp_layout_spec_t layout_4p = {
    .panel_count = 4,
    .panels = panels_4p,
    .angle_fn = get_4p_orientation_angle,
    .switch_font_by_orientation = true,
};

static const mp_layout_spec_t *get_layout(int track)
{
    if (track == 2) return &layout_2p;
    if (track == 3) return &layout_3p;
    return &layout_4p;
}

/* ---------- per-panel refresh ---------- */
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

/* ---------- unified refresh ---------- */
void refresh_multiplayer_ui(void)
{
    const mp_layout_spec_t *layout = mp_state.layout;
    int orientation_mode;
    int i;

    if (layout == NULL) return;
    orientation_mode = nvs_get_orientation();

    for (i = 0; i < layout->panel_count; i++) {
        const mp_panel_spec_t *spec = &layout->panels[i];
        lv_obj_t *panel = mp_state.panels[i];
        lv_obj_t *life_lbl = mp_state.life_labels[i];
        lv_obj_t *name_lbl = mp_state.name_labels[i];
        int16_t angle = layout->angle_fn(orientation_mode, i);
        int16_t counter_angle = get_counter_row_angle(orientation_mode, panel, angle);
        lv_coord_t nx = (orientation_mode != ORIENTATION_MODE_CENTRIC) ? spec->nudge_x : 0;
        lv_color_t text_color;

        text_color = refresh_mp_panel(panel, life_lbl, name_lbl,
                                      spec->player_index, spec->color_index);

        if (layout->switch_font_by_orientation) {
            if (life_lbl != NULL) {
                lv_obj_clear_flag(life_lbl, LV_OBJ_FLAG_HIDDEN);
                if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
                    lv_obj_set_style_text_font(life_lbl, &lv_font_montserrat_bold_44, 0);
                    lv_obj_align(life_lbl, LV_ALIGN_CENTER, nx, -10);
                } else {
                    lv_obj_set_style_text_font(life_lbl, &lv_font_montserrat_bold_56, 0);
                    lv_obj_align(life_lbl, LV_ALIGN_CENTER, nx, -12);
                }
            }
            if (name_lbl != NULL) {
                lv_obj_clear_flag(name_lbl, LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(name_lbl, LV_ALIGN_CENTER, nx, 30);
            }
            if (orientation_mode == ORIENTATION_MODE_CENTRIC) {
                apply_label_rotation(life_lbl, name_lbl, angle, 10, -30);
            } else {
                apply_label_rotation(life_lbl, name_lbl, angle, 12, -30);
            }
        } else {
            apply_label_rotation(life_lbl, name_lbl, angle, 10, -30);
        }

        refresh_counter_rows(panel, mp_state.counter_rows[i], mp_state.counter_values[i],
                             spec->player_index, text_color, angle, counter_angle);
    }
}

/* ---------- events ---------- */
static void event_multiplayer_select(lv_event_t *e)
{
    int player = (int)(intptr_t)lv_event_get_user_data(e);

    if (player < 0 || player >= MULTIPLAYER_COUNT) return;
    if (player_eliminated[player]) return;

    if (life_preview_active && preview_player != player) {
        life_preview_commit_cb(NULL);
    }

    if (selected_player == player) {
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
    int player = (int)(intptr_t)lv_event_get_user_data(e);

    if (player < 0 || player >= MULTIPLAYER_COUNT) return;
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

/* ---------- selection timeout ---------- */
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

/* ---------- layout rebuild ---------- */
void rebuild_multiplayer_layout(int track)
{
    const mp_layout_spec_t *layout = get_layout(track);
    int i;

    if (screen_multiplayer == NULL) return;

    lv_obj_clean(screen_multiplayer);
    memset(&mp_state, 0, sizeof(mp_state));
    mp_state.layout = layout;

    for (i = 0; i < layout->panel_count; i++) {
        const mp_panel_spec_t *spec = &layout->panels[i];
        int p = spec->player_index;
        lv_obj_t *panel;
        lv_obj_t *name_lbl;
        lv_obj_t *life_lbl;

        panel = lv_btn_create(screen_multiplayer);
        lv_obj_remove_style_all(panel);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_size(panel, spec->w, spec->h);
        lv_obj_set_pos(panel, spec->x, spec->y);
        lv_obj_set_style_radius(panel, 0, 0);
        lv_obj_set_style_border_width(panel, 1, 0);
        lv_obj_set_style_border_color(panel, lv_color_black(), 0);
        lv_obj_set_style_shadow_width(panel, 0, 0);
        lv_obj_add_event_cb(panel, event_multiplayer_select, LV_EVENT_CLICKED, (void *)(intptr_t)p);
        lv_obj_add_event_cb(panel, event_multiplayer_open_menu, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)p);
        mp_state.panels[i] = panel;

        name_lbl = lv_label_create(panel);
        lv_label_set_text(name_lbl, player_names[p]);
        lv_obj_set_style_text_color(name_lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_22, 0);
        lv_obj_align(name_lbl, LV_ALIGN_CENTER, 0, 30);
        mp_state.name_labels[i] = name_lbl;

        life_lbl = lv_label_create(panel);
        lv_label_set_text(life_lbl, "40");
        lv_obj_set_style_text_color(life_lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(life_lbl, &lv_font_montserrat_bold_56, 0);
        lv_obj_align(life_lbl, LV_ALIGN_CENTER, 0, -10);
        mp_state.life_labels[i] = life_lbl;

        create_counter_row(panel, COUNTER_TYPE_COMMANDER_TAX,
            &mp_state.counter_rows[i][COUNTER_TYPE_COMMANDER_TAX],
            &mp_state.counter_values[i][COUNTER_TYPE_COMMANDER_TAX], p);
        create_counter_row(panel, COUNTER_TYPE_PARTNER_TAX,
            &mp_state.counter_rows[i][COUNTER_TYPE_PARTNER_TAX],
            &mp_state.counter_values[i][COUNTER_TYPE_PARTNER_TAX], p);
        create_counter_row(panel, COUNTER_TYPE_POISON,
            &mp_state.counter_rows[i][COUNTER_TYPE_POISON],
            &mp_state.counter_values[i][COUNTER_TYPE_POISON], p);
        create_counter_row(panel, COUNTER_TYPE_EXPERIENCE,
            &mp_state.counter_rows[i][COUNTER_TYPE_EXPERIENCE],
            &mp_state.counter_values[i][COUNTER_TYPE_EXPERIENCE], p);
    }

    refresh_multiplayer_ui();
}

/* ---------- screen lifecycle ---------- */
void build_multiplayer_screen(void)
{
    screen_multiplayer = lv_obj_create(NULL);
    lv_obj_set_size(screen_multiplayer, 360, 360);
    lv_obj_set_style_bg_color(screen_multiplayer, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_multiplayer, 0, 0);
    lv_obj_set_scrollbar_mode(screen_multiplayer, LV_SCROLLBAR_MODE_OFF);

    rebuild_multiplayer_layout(nvs_get_players_to_track());
}

void open_multiplayer_screen(void)
{
    refresh_multiplayer_ui();
    load_screen_if_needed(screen_multiplayer);
}
