#include "knob_scr_main.h"
#include "knob_life.h"
#include "knob_timer.h"
#include "knob_nvs.h"

// Forward declarations for multiplayer routing
extern lv_obj_t *screen_4p;
extern void refresh_multiplayer_ui(void);

// ---------- screens ----------
lv_obj_t *screen_1p = NULL;
lv_obj_t *screen_select = NULL;
lv_obj_t *screen_damage = NULL;

// ---------- main UI widgets ----------
static lv_obj_t *arc_life = NULL;
static lv_obj_t *life_container = NULL;
static lv_obj_t *life_hitbox = NULL;
static lv_obj_t *turn_container = NULL;
static lv_obj_t *label_turn = NULL;
static lv_obj_t *label_turn_time = NULL;
static lv_obj_t *turn_live_dot = NULL;

// 7-segment digits
static lv_obj_t *digit_hundreds[7];
static lv_obj_t *digit_tens[7];
static lv_obj_t *digit_ones[7];
static lv_obj_t *digit_sign[7];
static lv_obj_t *digit_sign_plus_vert = NULL;

static lv_obj_t *digit_box_sign = NULL;
static lv_obj_t *digit_box_hundreds = NULL;
static lv_obj_t *digit_box_tens = NULL;
static lv_obj_t *digit_box_ones = NULL;

// ---------- select UI ----------
static lv_obj_t *label_select_title = NULL;
static lv_obj_t *select_rows[MAX_ENEMY_COUNT];
static lv_obj_t *label_enemy_name[MAX_ENEMY_COUNT];
static lv_obj_t *label_enemy_damage[MAX_ENEMY_COUNT];

// ---------- damage UI ----------
static lv_obj_t *label_damage_title = NULL;
static lv_obj_t *label_damage_value = NULL;
static lv_obj_t *label_damage_hint = NULL;

// ---------- 7-segment helpers ----------
static void create_digit(lv_obj_t *parent, lv_obj_t **seg)
{
    seg[0] = make_seg(parent, 10,   0, 40,  8);  // top
    seg[1] = make_seg(parent,  0,   8,  8, 44);  // upper-left
    seg[2] = make_seg(parent, 52,   8,  8, 44);  // upper-right
    seg[3] = make_seg(parent, 10,  52, 40,  8);  // middle
    seg[4] = make_seg(parent,  0,  60,  8, 44);  // lower-left
    seg[5] = make_seg(parent, 52,  60,  8, 44);  // lower-right
    seg[6] = make_seg(parent, 10, 104, 40,  8);  // bottom
}

static void set_seg_style(lv_obj_t *seg, lv_color_t color, bool on)
{
    if (on) {
        lv_obj_set_style_bg_color(seg, color, 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_color(seg, lv_color_hex(0x101010), 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_30, 0);
    }
}

static void set_digit_segments(lv_obj_t **seg, int value, lv_color_t color, bool visible)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (!visible) {
            lv_obj_add_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
            set_seg_style(seg[i], color, seg_map[value][i] ? true : false);
        }
    }
}

static void set_minus_segments(lv_obj_t **seg, lv_color_t color, bool visible)
{
    int i;

    if (digit_sign_plus_vert != NULL) {
        lv_obj_add_flag(digit_sign_plus_vert, LV_OBJ_FLAG_HIDDEN);
    }

    for (i = 0; i < 7; i++) {
        if (!visible) {
            lv_obj_add_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
            set_seg_style(seg[i], color, (i == 3));
        }
    }
}

static void set_plus_segments(lv_obj_t **seg, lv_color_t color, bool visible)
{
    int i;

    if (digit_sign_plus_vert != NULL) {
        if (!visible) {
            lv_obj_add_flag(digit_sign_plus_vert, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(digit_sign_plus_vert, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(digit_sign_plus_vert, color, 0);
            lv_obj_set_style_bg_opa(digit_sign_plus_vert, LV_OPA_COVER, 0);
        }
    }

    for (i = 0; i < 7; i++) {
        if (!visible) {
            lv_obj_add_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(seg[i], LV_OBJ_FLAG_HIDDEN);
            set_seg_style(seg[i], color, (i == 3));
        }
    }
}

// ---------- refresh functions ----------
static void refresh_ring(void)
{
    int max_life = nvs_get_life_total();
    lv_color_t c = get_life_color(life_total, max_life);

    lv_arc_set_range(arc_life, 0, max_life);
    lv_arc_set_value(arc_life, get_arc_display_value(life_total, max_life));

    lv_obj_set_style_arc_color(arc_life, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_life, 20, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc_life, c, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_life, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_life, true, LV_PART_INDICATOR);
}

void refresh_turn_ui(void)
{
    char turn_buf[24];
    char time_buf[24];
    uint32_t total_seconds = get_turn_elapsed_ms() / 1000;
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;

    if (turn_number <= 0) {
        lv_label_set_text(label_turn, "turn");
    } else {
        snprintf(turn_buf, sizeof(turn_buf), "turn %d", turn_number);
        lv_label_set_text(label_turn, turn_buf);
    }

    snprintf(time_buf, sizeof(time_buf), "%lu:%02lu",
             (unsigned long)hours, (unsigned long)minutes);
    lv_label_set_text(label_turn_time, time_buf);

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
    int display_value = life_preview_active ? pending_life_delta : life_total;
    int abs_v = (display_value < 0) ? -display_value : display_value;
    int hundreds = abs_v / 100;
    int tens = (abs_v / 10) % 10;
    int ones = abs_v % 10;
    bool negative = (display_value < 0);
    bool positive_preview = life_preview_active && (display_value > 0);
    lv_coord_t x_offset = 0;
    lv_color_t c = life_preview_active ? (negative ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_GREEN))
                                   : get_life_color(display_value, nvs_get_life_total());

    if (positive_preview) {
        if (abs_v >= 100) x_offset = -34;
        else if (abs_v >= 10) x_offset = -12;
        else x_offset = 0;
    } else if (negative) {
        if (abs_v >= 100) x_offset = -25;
        else x_offset = 0;
    }

    if (life_container != NULL) {
        lv_obj_align(life_container, LV_ALIGN_CENTER, x_offset, -6);
    }
    if (life_hitbox != NULL) {
        lv_obj_align(life_hitbox, LV_ALIGN_CENTER, x_offset, -8);
    }

    lv_obj_add_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(digit_box_hundreds, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
    set_minus_segments(digit_sign, c, false);
    set_plus_segments(digit_sign, c, false);

    if (positive_preview && abs_v >= 100) {
        lv_obj_set_pos(digit_box_sign, 24, 0);
        lv_obj_set_pos(digit_box_hundreds, 82, 0);
        lv_obj_set_pos(digit_box_tens, 152, 0);
        lv_obj_set_pos(digit_box_ones, 222, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_hundreds, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_plus_segments(digit_sign, c, true);
        set_digit_segments(digit_hundreds, hundreds, c, true);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (positive_preview && abs_v >= 10) {
        lv_obj_set_pos(digit_box_sign, 54, 0);
        lv_obj_set_pos(digit_box_tens, 112, 0);
        lv_obj_set_pos(digit_box_ones, 182, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_plus_segments(digit_sign, c, true);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (positive_preview) {
        lv_obj_set_pos(digit_box_sign, 86, 0);
        lv_obj_set_pos(digit_box_ones, 150, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_plus_segments(digit_sign, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (negative && abs_v >= 100) {
        lv_obj_set_pos(digit_box_sign, 0, 0);
        lv_obj_set_pos(digit_box_hundreds, 70, 0);
        lv_obj_set_pos(digit_box_tens, 140, 0);
        lv_obj_set_pos(digit_box_ones, 210, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_hundreds, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_minus_segments(digit_sign, c, true);
        set_digit_segments(digit_hundreds, hundreds, c, true);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (negative && abs_v >= 10) {
        lv_obj_set_pos(digit_box_sign, 10, 0);
        lv_obj_set_pos(digit_box_tens, 80, 0);
        lv_obj_set_pos(digit_box_ones, 150, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_minus_segments(digit_sign, c, true);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (negative) {
        lv_obj_set_pos(digit_box_sign, 45, 0);
        lv_obj_set_pos(digit_box_ones, 115, 0);
        lv_obj_clear_flag(digit_box_sign, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_minus_segments(digit_sign, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (abs_v >= 100) {
        lv_obj_set_pos(digit_box_hundreds, 45, 0);
        lv_obj_set_pos(digit_box_tens, 115, 0);
        lv_obj_set_pos(digit_box_ones, 185, 0);
        lv_obj_clear_flag(digit_box_hundreds, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_digit_segments(digit_hundreds, hundreds, c, true);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else if (abs_v >= 10) {
        lv_obj_set_pos(digit_box_tens, 80, 0);
        lv_obj_set_pos(digit_box_ones, 150, 0);
        lv_obj_clear_flag(digit_box_tens, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_digit_segments(digit_tens, tens, c, true);
        set_digit_segments(digit_ones, ones, c, true);
    }
    else {
        lv_obj_set_pos(digit_box_ones, 115, 0);
        lv_obj_clear_flag(digit_box_ones, LV_OBJ_FLAG_HIDDEN);
        set_digit_segments(digit_ones, ones, c, true);
    }
}

void refresh_main_ui(void)
{
    refresh_ring();
    refresh_life_digits();
    refresh_turn_ui();
}

void refresh_select_ui(void)
{
    char buf[32];
    int i;
    int n = active_enemy_count;
    int row_h = 46;
    int gap = 10;
    int total_h;
    int start_y;

    if (n > 5) { row_h = 38; gap = 6; }
    total_h = n * row_h + (n > 0 ? (n - 1) * gap : 0);
    start_y = (260 - total_h) / 2;

    for (i = 0; i < MAX_ENEMY_COUNT; i++) {
        if (select_rows[i] == NULL) continue;

        if (i < n) {
            int player_index = get_cmd_target_player_index(i);
            lv_color_t text_color = get_player_text_color(player_index);

            lv_obj_clear_flag(select_rows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(select_rows[i], 220, row_h);
            lv_obj_set_pos(select_rows[i], 0, start_y + i * (row_h + gap));

            lv_label_set_text(label_enemy_name[i], multiplayer_names[player_index]);
            snprintf(buf, sizeof(buf), "%d", enemies[i].damage);
            lv_label_set_text(label_enemy_damage[i], buf);
            lv_obj_set_style_text_color(label_enemy_name[i], text_color, 0);
            lv_obj_set_style_text_color(label_enemy_damage[i], text_color, 0);
            lv_obj_set_style_bg_color(select_rows[i], get_player_base_color(player_index), 0);
            lv_obj_set_style_bg_opa(select_rows[i], LV_OPA_COVER, 0);
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
        lv_label_set_text(label_damage_title, multiplayer_names[player_index]);
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
        extern lv_obj_t *screen_2p;
        extern lv_obj_t *screen_3p;
        refresh_multiplayer_ui();
        if (track == 2) load_screen_if_needed(screen_2p);
        else if (track == 3) load_screen_if_needed(screen_3p);
        else load_screen_if_needed(screen_4p);
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
static void event_open_select(lv_event_t *e)
{
    (void)e;
    if (active_enemy_count <= 0) return;
    open_select_screen();
}

static void event_select_enemy(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
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
    lv_arc_set_value(arc_life, get_arc_display_value(life_total, nvs_get_life_total()));
    lv_obj_remove_style(arc_life, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_life, LV_OBJ_FLAG_CLICKABLE);

    life_hitbox = make_plain_box(screen_1p, 320, 188);
    lv_obj_align(life_hitbox, LV_ALIGN_CENTER, 0, -8);
    lv_obj_add_flag(life_hitbox, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(life_hitbox, event_open_select, LV_EVENT_LONG_PRESSED, NULL);

    life_container = make_plain_box(screen_1p, 290, 112);
    lv_obj_align(life_container, LV_ALIGN_CENTER, 0, -6);

    digit_box_sign = make_plain_box(life_container, 60, 112);
    create_digit(digit_box_sign, digit_sign);
    digit_sign_plus_vert = make_seg(digit_box_sign, 27, 34, 6, 40);
    lv_obj_set_style_radius(digit_sign_plus_vert, 2, 0);
    lv_obj_add_flag(digit_sign_plus_vert, LV_OBJ_FLAG_HIDDEN);

    digit_box_hundreds = make_plain_box(life_container, 60, 112);
    create_digit(digit_box_hundreds, digit_hundreds);

    digit_box_tens = make_plain_box(life_container, 60, 112);
    create_digit(digit_box_tens, digit_tens);

    digit_box_ones = make_plain_box(life_container, 60, 112);
    create_digit(digit_box_ones, digit_ones);

    turn_container = make_plain_box(screen_1p, 96, 96);
    lv_obj_align(turn_container, LV_ALIGN_CENTER, 110, -6);
    lv_obj_add_flag(turn_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(turn_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(turn_container, event_turn_tap, LV_EVENT_CLICKED, NULL);

    label_turn = lv_label_create(turn_container);
    lv_label_set_text(label_turn, "turn");
    lv_obj_set_style_text_color(label_turn, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_turn, &lv_font_montserrat_22, 0);
    lv_obj_align(label_turn, LV_ALIGN_TOP_MID, 0, 10);

    label_turn_time = lv_label_create(turn_container);
    lv_label_set_text(label_turn_time, "0:00");
    lv_obj_set_style_text_color(label_turn_time, lv_color_hex(0xB8B8B8), 0);
    lv_obj_set_style_text_font(label_turn_time, &lv_font_montserrat_22, 0);
    lv_obj_align(label_turn_time, LV_ALIGN_TOP_MID, -8, 48);

    turn_live_dot = lv_obj_create(turn_container);
    lv_obj_remove_style_all(turn_live_dot);
    lv_obj_set_size(turn_live_dot, 12, 12);
    lv_obj_set_style_radius(turn_live_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(turn_live_dot, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(turn_live_dot, LV_OPA_COVER, 0);
    lv_obj_align(turn_live_dot, LV_ALIGN_TOP_MID, 28, 52);
    lv_obj_add_flag(turn_live_dot, LV_OBJ_FLAG_HIDDEN);
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
