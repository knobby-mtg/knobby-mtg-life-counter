#include "ui_1p.h"
#include "settings.h"
#include "ui_mp.h"
#include "ui_player_menu.h"
#include "game.h"
#include "timer.h"
#include "storage.h"

// ---------- screens ----------
lv_obj_t *screen_1p = NULL;
lv_obj_t *screen_select = NULL;
lv_obj_t *screen_damage = NULL;

// ---------- main UI widgets ----------
static lv_obj_t *arc_life = NULL;
static lv_obj_t *life_hitbox = NULL;
static lv_obj_t *label_life_total = NULL;
static lv_obj_t *label_life_preview_total = NULL;
static lv_obj_t *turn_container = NULL;
static lv_obj_t *label_turn = NULL;
static lv_obj_t *turn_live_dot = NULL;

// ---------- 1p counter widgets ----------
static lv_obj_t *counter_row_1p[COUNTER_TYPE_COUNT];
static lv_obj_t *counter_value_1p[COUNTER_TYPE_COUNT];

// ---------- select UI ----------
static lv_obj_t *label_select_title = NULL;
static lv_obj_t *select_rows[MAX_ENEMY_COUNT];
static lv_obj_t *label_enemy_name[MAX_ENEMY_COUNT];
static lv_obj_t *label_enemy_damage[MAX_ENEMY_COUNT];

// ---------- damage UI ----------
static lv_obj_t *label_damage_title = NULL;
static lv_obj_t *label_damage_value = NULL;
static lv_obj_t *label_damage_hint = NULL;

// ---------- refresh functions ----------
static void refresh_ring(void)
{
    int max_life = nvs_get_life_total();
    lv_color_t c = get_effective_player_color(0, 0, LIFE_VIB_MID);

    lv_arc_set_range(arc_life, 0, max_life);
    lv_arc_set_value(arc_life, get_arc_display_value(player_life[0], max_life));

    lv_obj_set_style_arc_color(arc_life, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_life, 20, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc_life, c, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_life, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_life, true, LV_PART_INDICATOR);
}

void refresh_turn_ui(void)
{
    char buf[48];
    uint32_t total_seconds = get_turn_elapsed_ms() / 1000;
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;

    if (turn_number <= 0) {
        snprintf(buf, sizeof(buf), "turn  %lu:%02lu",
                 (unsigned long)hours, (unsigned long)minutes);
    } else {
        snprintf(buf, sizeof(buf), "turn %d  %lu:%02lu",
                 turn_number, (unsigned long)hours, (unsigned long)minutes);
    }
    lv_label_set_text(label_turn, buf);

    if (turn_live_dot != NULL) {
        lv_obj_align_to(turn_live_dot, label_turn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    }

    if (turn_container != NULL) {
        if (turn_ui_visible) {
            lv_obj_clear_flag(turn_container, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(turn_container, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (turn_live_dot != NULL) {
        if (turn_timer_enabled) {
            lv_obj_clear_flag(turn_live_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(turn_live_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (turn_container != NULL) {
        lv_obj_set_style_opa(turn_container, turn_indicator_visible ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
}

static void refresh_life_digits(void)
{
    int display_value = life_preview_active ? pending_life_delta : player_life[0];
    bool negative = (display_value < 0);
    lv_color_t c;
    char buf[16];

    if (life_preview_active) {
        c = negative ? lv_color_hex(0xFF1744)
                     : lv_color_hex(0x06D6A0);
        if (display_value > 0)
            snprintf(buf, sizeof(buf), "+%d", display_value);
        else
            snprintf(buf, sizeof(buf), "%d", display_value);
    } else {
        c = get_effective_player_color(0, 0, LIFE_VIB_MID);
        snprintf(buf, sizeof(buf), "%d", display_value);
    }

    lv_label_set_text(label_life_total, buf);
    lv_obj_set_style_text_color(label_life_total, c, 0);
    lv_obj_align(label_life_total, LV_ALIGN_CENTER, 0, -6);

    if (life_preview_active && label_life_preview_total != NULL) {
        int new_total = player_life[0] + pending_life_delta;
        snprintf(buf, sizeof(buf), "= %d", new_total);
        lv_label_set_text(label_life_preview_total, buf);
        lv_obj_clear_flag(label_life_preview_total, LV_OBJ_FLAG_HIDDEN);
    } else if (label_life_preview_total != NULL) {
        lv_obj_add_flag(label_life_preview_total, LV_OBJ_FLAG_HIDDEN);
    }
}

static void refresh_1p_counters(void)
{
    int type;
    int visible_count = 0;
    int visible_types[COUNTER_TYPE_COUNT];
    char buf[8];
    const lv_coord_t step = 30;
    const lv_coord_t counter_y = 46;
    lv_color_t text_color = lv_color_white();

    for (type = 0; type < COUNTER_TYPE_COUNT; type++) {
        if (counter_row_1p[type] == NULL || counter_value_1p[type] == NULL) continue;

        if (!counter_type_is_enabled((counter_type_t)type) ||
            get_counter_value(0, (counter_type_t)type) <= 0) {
            lv_obj_add_flag(counter_row_1p[type], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        visible_types[visible_count] = type;
        visible_count++;
    }

    for (type = 0; type < visible_count; type++) {
        int value;
        int counter_type = visible_types[type];
        lv_coord_t x_offset = (lv_coord_t)((type * step) - ((visible_count - 1) * step / 2));

        value = get_counter_value(0, (counter_type_t)counter_type);

        snprintf(buf, sizeof(buf), "%d", value);
        lv_label_set_text(counter_value_1p[counter_type], buf);
        lv_obj_set_style_text_color(counter_value_1p[counter_type], text_color, 0);
        lv_obj_clear_flag(counter_row_1p[counter_type], LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(counter_row_1p[counter_type], LV_ALIGN_TOP_MID, x_offset, counter_y);
    }

}

void refresh_main_ui(void)
{
    refresh_ring();
    refresh_life_digits();
    refresh_turn_ui();
    refresh_1p_counters();
}

void refresh_player_ui(void)
{
    if (nvs_get_players_to_track() == 1)
        refresh_main_ui();
    else
        refresh_multiplayer_ui();
}

/* grid constants for select screen */
#define SEL_BOX_W      100
#define SEL_BOX_H       56
#define SEL_BOX_GAP_X    6
#define SEL_BOX_GAP_Y    6
#define SEL_GRID_COLS    2

static bool is_enemy_eliminated(int player_index)
{
    return (player_index >= 0 && player_index < MAX_DISPLAY_PLAYERS &&
            player_eliminated[player_index]);
}

static void style_select_entry(int i, int player_index)
{
    char buf[32];
    lv_color_t text_color = get_player_text_color(player_index);
    bool eliminated = is_enemy_eliminated(player_index);

    lv_label_set_text(label_enemy_name[i], player_names[player_index]);
    snprintf(buf, sizeof(buf), "%d", enemies[i].damage);
    lv_label_set_text(label_enemy_damage[i], buf);

    if (eliminated) {
        lv_color_t disabled_bg = lv_color_hex(0x404040);
        lv_color_t disabled_text = lv_color_hex(0x808080);
        lv_obj_set_style_text_color(label_enemy_name[i], disabled_text, 0);
        lv_obj_set_style_text_color(label_enemy_damage[i], disabled_text, 0);
        lv_obj_set_style_bg_color(select_rows[i], disabled_bg, 0);
        lv_obj_set_style_bg_opa(select_rows[i], LV_OPA_COVER, 0);
        lv_obj_clear_flag(select_rows[i], LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_set_style_text_color(label_enemy_name[i], text_color, 0);
        lv_obj_set_style_text_color(label_enemy_damage[i], text_color, 0);
        lv_obj_set_style_bg_color(select_rows[i], get_player_base_color(player_index), 0);
        lv_obj_set_style_bg_opa(select_rows[i], LV_OPA_COVER, 0);
        lv_obj_clear_flag(select_rows[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(select_rows[i], LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_set_style_text_font(label_enemy_name[i], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(label_enemy_damage[i], &lv_font_montserrat_22, 0);
    lv_obj_align(label_enemy_name[i], LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_align(label_enemy_damage[i], LV_ALIGN_BOTTOM_MID, 0, -4);
}

void refresh_select_ui(void)
{
    int i;
    int n = active_enemy_count;
    int cols = (n <= 1) ? 1 : SEL_GRID_COLS;
    int rows_needed = (n + cols - 1) / cols;
    int grid_w = cols * SEL_BOX_W + (cols - 1) * SEL_BOX_GAP_X;
    int grid_h = rows_needed * SEL_BOX_H + (rows_needed - 1) * SEL_BOX_GAP_Y;
    int grid_x = (240 - grid_w) / 2;
    int grid_y = (260 - grid_h) / 2;

    for (i = 0; i < MAX_ENEMY_COUNT; i++) {
        if (select_rows[i] == NULL) continue;

        if (i < n) {
            int col = i % cols;
            int row = i / cols;
            int items_in_row = (row < n / cols) ? cols : (n % cols);
            if (items_in_row == 0) items_in_row = cols;
            int row_offset = (cols > 1 && items_in_row == 1) ? (SEL_BOX_W + SEL_BOX_GAP_X) / 2 : 0;
            int x = grid_x + col * (SEL_BOX_W + SEL_BOX_GAP_X) + row_offset;
            int y = grid_y + row * (SEL_BOX_H + SEL_BOX_GAP_Y);
            int player_index = get_cmd_target_player_index(i);

            lv_obj_clear_flag(select_rows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(select_rows[i], SEL_BOX_W, SEL_BOX_H);
            lv_obj_set_pos(select_rows[i], x, y);
            lv_obj_set_style_radius(select_rows[i], 8, 0);
            style_select_entry(i, player_index);
        } else {
            lv_obj_add_flag(select_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void refresh_damage_ui(void)
{
    char buf[64];
    lv_color_t text_color;

    if (selected_enemy < 0 || selected_enemy >= active_enemy_count) return;

    {
        int player_index = get_cmd_target_player_index(selected_enemy);
        text_color = get_player_text_color(player_index);
        lv_label_set_text(label_damage_title, player_names[player_index]);
        lv_obj_set_style_bg_color(screen_damage, get_player_active_color(player_index), 0);
    }
    lv_obj_set_style_text_color(label_damage_title, text_color, 0);
    lv_obj_set_style_text_color(label_damage_value, text_color, 0);
    lv_obj_set_style_text_color(label_damage_hint, text_color, 0);

    snprintf(buf, sizeof(buf), "Damage: %d", enemies[selected_enemy].damage);
    lv_label_set_text(label_damage_value, buf);
}

// ---------- navigation ----------
void back_to_main(void)
{
    int track = nvs_get_players_to_track();
    cmd_damage_target = -1;
    if (track > 1) {
        refresh_multiplayer_ui();
        load_screen_if_needed(screen_multiplayer);
    } else {
        refresh_main_ui();
        load_screen_if_needed(screen_1p);
    }
}

void open_select_screen(void)
{
    refresh_select_ui();
    load_screen_if_needed(screen_select);
}

static void open_damage_screen(int enemy_index)
{
    selected_enemy = enemy_index;
    damage_enter();
    refresh_damage_ui();
    load_screen_if_needed(screen_damage);
}

// ---------- events ----------
static void event_open_1p_menu(lv_event_t *e)
{
    (void)e;
    open_player_menu(0);
}

static void event_select_enemy(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    int player_index = get_cmd_target_player_index(index);
    if (player_index >= 0 && player_index < MAX_DISPLAY_PLAYERS && player_eliminated[player_index]) {
        return;
    }
    open_damage_screen(index);
}

static void event_damage_apply(lv_event_t *e)
{
    (void)e;
    bool was_cmd = (cmd_damage_target >= 0);
    damage_apply();
    if (was_cmd) {
        back_to_main();
    } else {
        refresh_select_ui();
        load_screen_if_needed(screen_select);
    }
}

static void event_back_main(lv_event_t *e)
{
    (void)e;
    back_to_main();
}

// ---------- counter row helper ----------
static const lv_font_t *get_counter_badge_font_1p(const counter_definition_t *definition)
{
    if (definition != NULL && definition->icon_text != NULL) {
        return &mana_counter_icons_16;
    }

    return &lv_font_montserrat_14;
}

static const char *get_counter_badge_text_1p(const counter_definition_t *definition)
{
    if (definition == NULL) return "?";
    if (definition->icon_text != NULL) return definition->icon_text;
    if (definition->badge_text != NULL) return definition->badge_text;
    return "?";
}

static void create_counter_row_1p(lv_obj_t *parent, counter_type_t type,
                                  lv_obj_t **row_out, lv_obj_t **value_out)
{
    const counter_definition_t *definition = get_counter_definition(type);
    lv_obj_t *row;
    lv_obj_t *glyph;

    row = make_plain_box(parent, 34, 34);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

    glyph = lv_label_create(row);
    lv_label_set_text(glyph, get_counter_badge_text_1p(definition));
    lv_obj_set_style_text_color(glyph, lv_color_white(), 0);
    lv_obj_set_style_text_font(glyph,
        (type == COUNTER_TYPE_POISON) ? &mana_poison_icon_bold_16
                                      : get_counter_badge_font_1p(definition), 0);
    lv_obj_align(glyph, LV_ALIGN_TOP_MID, 0, 0);

    *value_out = lv_label_create(row);
    lv_label_set_text(*value_out, "0");
    lv_obj_set_style_text_color(*value_out, lv_color_white(), 0);
    lv_obj_set_style_text_font(*value_out, &lv_font_montserrat_14, 0);
    lv_obj_align(*value_out, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(*value_out, LV_TEXT_ALIGN_CENTER, 0);

    *row_out = row;
}

// ---------- screen builders ----------
void build_main_screen(void)
{
    screen_1p = lv_obj_create(NULL);
    lv_obj_set_size(screen_1p, 360, 360);
    lv_obj_set_style_bg_color(screen_1p, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_1p, 0, 0);
    lv_obj_set_scrollbar_mode(screen_1p, LV_SCROLLBAR_MODE_OFF);

    arc_life = lv_arc_create(screen_1p);
    lv_obj_set_size(arc_life, 360, 360);
    lv_obj_center(arc_life);
    lv_arc_set_rotation(arc_life, 90);
    lv_arc_set_bg_angles(arc_life, 0, 360);
    lv_arc_set_range(arc_life, 0, nvs_get_life_total());
    lv_arc_set_value(arc_life, get_arc_display_value(player_life[0], nvs_get_life_total()));
    lv_obj_remove_style(arc_life, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_life, LV_OBJ_FLAG_CLICKABLE);

    life_hitbox = make_plain_box(screen_1p, 360, 360);
    lv_obj_align(life_hitbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(life_hitbox, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(life_hitbox, event_open_1p_menu, LV_EVENT_LONG_PRESSED, NULL);

    label_life_total = lv_label_create(screen_1p);
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", player_life[0]);
        lv_label_set_text(label_life_total, buf);
    }
    lv_obj_set_style_text_font(label_life_total, &lv_font_montserrat_bold_116, 0);
    lv_obj_set_style_text_color(label_life_total, lv_color_white(), 0);
    lv_obj_align(label_life_total, LV_ALIGN_CENTER, 0, -6);

    label_life_preview_total = lv_label_create(screen_1p);
    lv_label_set_text(label_life_preview_total, "");
    lv_obj_set_style_text_color(label_life_preview_total, lv_color_hex(0x06D6A0), 0);
    lv_obj_set_style_text_font(label_life_preview_total, &lv_font_montserrat_regular_48, 0);
    lv_obj_align(label_life_preview_total, LV_ALIGN_CENTER, 0, 80);
    lv_obj_add_flag(label_life_preview_total, LV_OBJ_FLAG_HIDDEN);

    turn_container = make_plain_box(screen_1p, 240, 32);
    lv_obj_align(turn_container, LV_ALIGN_CENTER, 0, 120);
    lv_obj_add_flag(turn_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(turn_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(turn_container, event_turn_tap, LV_EVENT_CLICKED, NULL);

    label_turn = lv_label_create(turn_container);
    lv_label_set_text(label_turn, "turn  0:00");
    lv_obj_set_style_text_color(label_turn, lv_color_hex(0xB8B8B8), 0);
    lv_obj_set_style_text_font(label_turn, &lv_font_montserrat_22, 0);
    lv_obj_align(label_turn, LV_ALIGN_CENTER, 0, 0);

    turn_live_dot = lv_obj_create(turn_container);
    lv_obj_remove_style_all(turn_live_dot);
    lv_obj_set_size(turn_live_dot, 10, 10);
    lv_obj_set_style_radius(turn_live_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(turn_live_dot, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(turn_live_dot, LV_OPA_COVER, 0);
    lv_obj_align_to(turn_live_dot, label_turn, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    lv_obj_add_flag(turn_live_dot, LV_OBJ_FLAG_HIDDEN);

    create_counter_row_1p(screen_1p, COUNTER_TYPE_COMMANDER_TAX,
        &counter_row_1p[COUNTER_TYPE_COMMANDER_TAX],
        &counter_value_1p[COUNTER_TYPE_COMMANDER_TAX]);
    create_counter_row_1p(screen_1p, COUNTER_TYPE_PARTNER_TAX,
        &counter_row_1p[COUNTER_TYPE_PARTNER_TAX],
        &counter_value_1p[COUNTER_TYPE_PARTNER_TAX]);
    create_counter_row_1p(screen_1p, COUNTER_TYPE_POISON,
        &counter_row_1p[COUNTER_TYPE_POISON],
        &counter_value_1p[COUNTER_TYPE_POISON]);
    create_counter_row_1p(screen_1p, COUNTER_TYPE_EXPERIENCE,
        &counter_row_1p[COUNTER_TYPE_EXPERIENCE],
        &counter_value_1p[COUNTER_TYPE_EXPERIENCE]);

}

void build_select_screen(void)
{
    int i;
    lv_obj_t *container;

    screen_select = lv_obj_create(NULL);
    lv_obj_set_size(screen_select, 360, 360);
    lv_obj_set_style_bg_color(screen_select, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_select, 0, 0);
    lv_obj_set_scrollbar_mode(screen_select, LV_SCROLLBAR_MODE_OFF);

    label_select_title = lv_label_create(screen_select);
    lv_label_set_text(label_select_title, "Choose player");
    lv_obj_set_style_text_color(label_select_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_select_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_select_title, LV_ALIGN_TOP_MID, 0, 30);

    container = lv_obj_create(screen_select);
    lv_obj_set_size(container, 240, 260);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_AUTO);

    for (i = 0; i < MAX_ENEMY_COUNT; i++) {
        select_rows[i] = lv_btn_create(container);
        lv_obj_remove_style_all(select_rows[i]);
        lv_obj_set_style_bg_opa(select_rows[i], LV_OPA_COVER, 0);
        lv_obj_set_size(select_rows[i], 220, 46);
        lv_obj_set_pos(select_rows[i], 0, i * 56);
        lv_obj_add_event_cb(select_rows[i], event_select_enemy, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        label_enemy_name[i] = lv_label_create(select_rows[i]);
        lv_obj_set_style_text_font(label_enemy_name[i], &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(label_enemy_name[i], lv_color_white(), 0);
        lv_obj_align(label_enemy_name[i], LV_ALIGN_LEFT_MID, 16, 0);

        label_enemy_damage[i] = lv_label_create(select_rows[i]);
        lv_obj_set_style_text_font(label_enemy_damage[i], &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(label_enemy_damage[i], lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_align(label_enemy_damage[i], LV_ALIGN_RIGHT_MID, -16, 0);
    }
}

void build_damage_screen(void)
{
    screen_damage = lv_obj_create(NULL);
    lv_obj_set_size(screen_damage, 360, 360);
    lv_obj_set_style_bg_color(screen_damage, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_damage, 0, 0);
    lv_obj_set_scrollbar_mode(screen_damage, LV_SCROLLBAR_MODE_OFF);

    label_damage_title = lv_label_create(screen_damage);
    lv_label_set_text(label_damage_title, "P1");
    lv_obj_set_style_text_color(label_damage_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_damage_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_damage_title, LV_ALIGN_TOP_MID, 0, 28);

    label_damage_value = lv_label_create(screen_damage);
    lv_label_set_text(label_damage_value, "Damage: 0");
    lv_obj_set_style_text_color(label_damage_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_damage_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_damage_value, LV_ALIGN_CENTER, 0, -10);

    label_damage_hint = lv_label_create(screen_damage);
    lv_label_set_text(label_damage_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_damage_hint, lv_color_hex(0x6A6A6A), 0);
    lv_obj_set_style_text_font(label_damage_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_damage_hint, LV_ALIGN_CENTER, 0, 24);

    lv_obj_t *btn = make_button(screen_damage, "Apply", 120, 46, event_damage_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}
