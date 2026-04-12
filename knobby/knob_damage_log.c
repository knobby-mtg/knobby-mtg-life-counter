#include "knob_damage_log.h"
#include "knob_life.h"

// ---------- data ----------
typedef struct {
    uint32_t timestamp_ms;
    int8_t   player;       // target: -1 = single player, 0-3 = multiplayer index
    int8_t   source;       // source for cmd damage or counter type, -1 if N/A
    uint8_t  event_type;   // log_event_type_t
    int16_t  delta;
} damage_log_entry_t;

static damage_log_entry_t damage_log[DAMAGE_LOG_MAX];
static int damage_log_count = 0;
static int damage_log_head = 0;

// ---------- screen ----------
lv_obj_t *screen_damage_log = NULL;
static lv_obj_t *damage_log_container = NULL;
static lv_obj_t *delete_btn = NULL;
static int damage_log_selected = -1;  // index into visible list (0 = newest)

// ---------- log operations ----------
void damage_log_add(int player, int delta, uint8_t event_type, int source)
{
    if (delta == 0) return;
    damage_log[damage_log_head].timestamp_ms = lv_tick_get();
    damage_log[damage_log_head].player     = (int8_t)player;
    damage_log[damage_log_head].source     = (int8_t)source;
    damage_log[damage_log_head].event_type = event_type;
    damage_log[damage_log_head].delta      = (int16_t)delta;
    damage_log_head = (damage_log_head + 1) % DAMAGE_LOG_MAX;
    if (damage_log_count < DAMAGE_LOG_MAX) damage_log_count++;
}

void damage_log_reset(void)
{
    damage_log_count = 0;
    damage_log_head = 0;
}

static void update_selection_highlight(void);
static void refresh_damage_log_ui(void);

void damage_log_select_next(void)
{
    if (damage_log_count == 0) return;
    if (damage_log_selected < damage_log_count - 1) {
        damage_log_selected++;
    }
    update_selection_highlight();
}

void damage_log_select_prev(void)
{
    if (damage_log_count == 0) return;
    if (damage_log_selected > 0) {
        damage_log_selected--;
    }
    update_selection_highlight();
}

void damage_log_undo_selected(void)
{
    int buf_idx, i;
    damage_log_entry_t *entry;

    if (damage_log_selected < 0 || damage_log_selected >= damage_log_count) return;

    buf_idx = (damage_log_head - 1 - damage_log_selected + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;
    entry = &damage_log[buf_idx];

    if (entry->event_type == LOG_EVT_LIFE) {
        undo_life_change(entry->player, entry->delta);
    } else if (entry->event_type == LOG_EVT_CMD_DAMAGE) {
        undo_life_change(entry->player, entry->delta);
        undo_cmd_damage(entry->source, entry->player, entry->delta);
    } else if (entry->event_type == LOG_EVT_COUNTER) {
        undo_counter_change(entry->player, entry->source, entry->delta);
    }

    /* Remove entry by shifting newer entries down */
    for (i = damage_log_selected; i > 0; i--) {
        int dst = (damage_log_head - 1 - i + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;
        int src = (damage_log_head - i + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;
        damage_log[dst] = damage_log[src];
    }
    damage_log_head = (damage_log_head - 1 + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;
    damage_log_count--;

    /* Adjust selection */
    if (damage_log_count == 0) {
        damage_log_selected = -1;
    } else if (damage_log_selected >= damage_log_count) {
        damage_log_selected = damage_log_count - 1;
    }

    refresh_damage_log_ui();
}

// ---------- UI ----------
static void format_elapsed(uint32_t elapsed_s, char *out, size_t out_sz)
{
    if (elapsed_s >= 60) {
        snprintf(out, out_sz, "%2lum ago", (unsigned long)(elapsed_s / 60));
    } else {
        snprintf(out, out_sz, "%2lus ago", (unsigned long)elapsed_s);
    }
}

static void format_log_line(damage_log_entry_t *entry, char *buf, size_t buf_sz)
{
    uint32_t elapsed_s = lv_tick_elaps(entry->timestamp_ms) / 1000;
    int abs_delta = entry->delta > 0 ? entry->delta : -entry->delta;
    char time_str[16];

    format_elapsed(elapsed_s, time_str, sizeof(time_str));

    if (entry->event_type == LOG_EVT_CMD_DAMAGE && entry->source >= 0) {
        snprintf(buf, buf_sz, "%s: %s dealt %d cmd to %s",
                 time_str,
                 multiplayer_names[entry->source],
                 abs_delta,
                 multiplayer_names[entry->player]);
    } else if (entry->event_type == LOG_EVT_COUNTER && entry->source >= 0 && entry->player >= 0) {
        const counter_definition_t *definition = get_counter_definition((counter_type_t)entry->source);
        const char *action = entry->delta > 0 ? "increased" : "decreased";
        const char *counter_name = (definition != NULL) ? definition->display_name : "Counter";
        snprintf(buf, buf_sz, "%s: %s %s %s by %d",
                 time_str,
                 multiplayer_names[entry->player],
                 action,
                 counter_name,
                 abs_delta);
    } else if (entry->player < 0) {
        const char *action = entry->delta > 0 ? "gained" : "lost";
        snprintf(buf, buf_sz, "%s: %s %d life", time_str, action, abs_delta);
    } else {
        const char *action = entry->delta > 0 ? "gained" : "lost";
        snprintf(buf, buf_sz, "%s: %s %s %d life",
                 time_str, multiplayer_names[entry->player], action, abs_delta);
    }
}

static void update_selection_highlight(void)
{
    int i;
    uint32_t child_count = lv_obj_get_child_cnt(damage_log_container);

    for (i = 0; i < (int)child_count && i < damage_log_count; i++) {
        int idx = (damage_log_head - 1 - i + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;
        lv_obj_t *lbl = lv_obj_get_child(damage_log_container, i);

        if (i == damage_log_selected) {
            lv_obj_set_style_bg_color(lbl, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
        }
    }

    /* Scroll selected item into view */
    if (damage_log_selected >= 0 && damage_log_selected < (int)child_count) {
        lv_obj_t *sel = lv_obj_get_child(damage_log_container, damage_log_selected);
        lv_coord_t sel_y = lv_obj_get_y(sel);
        lv_coord_t sel_h = lv_obj_get_height(sel);
        lv_coord_t cont_h = lv_obj_get_height(damage_log_container);
        lv_coord_t scroll_y = lv_obj_get_scroll_y(damage_log_container);

        if (sel_y - scroll_y < 0) {
            lv_obj_scroll_to_y(damage_log_container, sel_y, LV_ANIM_ON);
        } else if (sel_y + sel_h - scroll_y > cont_h) {
            lv_obj_scroll_to_y(damage_log_container, sel_y + sel_h - cont_h, LV_ANIM_ON);
        }
    }

    /* Show/hide delete button */
    if (delete_btn != NULL) {
        if (damage_log_selected >= 0) {
            lv_obj_clear_flag(delete_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(delete_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void refresh_damage_log_ui(void)
{
    int i, idx;
    char buf[80];

    lv_obj_clean(damage_log_container);

    if (damage_log_count == 0) {
        lv_obj_t *lbl = lv_label_create(damage_log_container);
        lv_label_set_text(lbl, "No events yet");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x7A7A7A), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        damage_log_selected = -1;
        update_selection_highlight();
        return;
    }

    for (i = 0; i < damage_log_count; i++) {
        idx = (damage_log_head - 1 - i + DAMAGE_LOG_MAX) % DAMAGE_LOG_MAX;

        format_log_line(&damage_log[idx], buf, sizeof(buf));

        lv_obj_t *lbl = lv_label_create(damage_log_container);
        lv_label_set_text(lbl, buf);
        lv_obj_set_width(lbl, 280);
        lv_obj_set_style_pad_left(lbl, 4, 0);
        lv_obj_set_style_pad_right(lbl, 4, 0);
        lv_obj_set_style_pad_top(lbl, 2, 0);
        lv_obj_set_style_pad_bottom(lbl, 2, 0);
        lv_obj_set_style_radius(lbl, 4, 0);
        if (damage_log[idx].event_type == LOG_EVT_COUNTER) {
            const counter_definition_t *definition = get_counter_definition((counter_type_t)damage_log[idx].source);
            lv_obj_set_style_text_color(lbl,
                definition != NULL ? lv_color_hex(definition->accent_color) : lv_color_hex(0xFFB74D), 0);
        } else {
            lv_obj_set_style_text_color(lbl,
                damage_log[idx].delta > 0 ? lv_color_hex(0x4CAF50) : lv_color_hex(0xFF5252), 0);
        }
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    }

    if (damage_log_selected >= damage_log_count) {
        damage_log_selected = damage_log_count - 1;
    }

    lv_obj_scroll_to_y(damage_log_container, 0, LV_ANIM_OFF);
    update_selection_highlight();
}

// ---------- navigation ----------
static void event_delete_pressed(lv_event_t *e)
{
    (void)e;
    damage_log_undo_selected();
}

void open_damage_log_screen(void)
{
    damage_log_selected = (damage_log_count > 0) ? 0 : -1;
    refresh_damage_log_ui();
    load_screen_if_needed(screen_damage_log);
}

static void event_open_damage_log(lv_event_t *e)
{
    (void)e;
    open_damage_log_screen();
}

// ---------- build ----------
void build_damage_log_screen(void)
{
    lv_obj_t *btn_label;

    screen_damage_log = lv_obj_create(NULL);
    lv_obj_set_size(screen_damage_log, 360, 360);
    lv_obj_set_style_bg_color(screen_damage_log, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_damage_log, 0, 0);
    lv_obj_set_scrollbar_mode(screen_damage_log, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen_damage_log);
    lv_label_set_text(title, "Event Log");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    damage_log_container = lv_obj_create(screen_damage_log);
    lv_obj_remove_style_all(damage_log_container);
    lv_obj_set_size(damage_log_container, 300, 174);
    lv_obj_align(damage_log_container, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_flex_flow(damage_log_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(damage_log_container, 2, 0);
    lv_obj_set_scrollbar_mode(damage_log_container, LV_SCROLLBAR_MODE_OFF);

    /* Delete / Undo button */
    delete_btn = lv_btn_create(screen_damage_log);
    lv_obj_remove_style_all(delete_btn);
    lv_obj_set_size(delete_btn, 140, 48);
    lv_obj_align(delete_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_ext_click_area(delete_btn, 20);
    lv_obj_set_style_bg_color(delete_btn, lv_color_hex(0xB71C1C), 0);
    lv_obj_set_style_bg_opa(delete_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(delete_btn, 6, 0);
    lv_obj_add_event_cb(delete_btn, event_delete_pressed, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_flag(delete_btn, LV_OBJ_FLAG_HIDDEN);

    btn_label = lv_label_create(delete_btn);
    lv_label_set_text(btn_label, "Undo\n(Long Press)");
    lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_label);

}
