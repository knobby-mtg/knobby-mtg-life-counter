#include "rename.h"
#include "ui_mp.h"
#include "ui_player_menu.h"
#include "ui_1p.h"
#include "game.h"
#include "storage.h"
#include <string.h>

// ---------- screens ----------
lv_obj_t *screen_player_name = NULL;

// ---------- MRU name list ----------
static char mru_names[NAME_LIST_COUNT][NAME_LIST_LEN];
static int mru_count = 0;

// ---------- widgets ----------
static lv_obj_t *label_name_title = NULL;
static lv_obj_t *mru_list_container = NULL;
static lv_obj_t *btn_mru_select = NULL;
static lv_obj_t *textarea_name = NULL;
static lv_obj_t *keyboard_name = NULL;
static lv_obj_t *btn_name_save = NULL;
static bool name_keyboard_mode = false;
static int mru_selected = 0;

// ---------- rename-all state ----------
static bool rename_all_active = false;
static int rename_all_start = 0;
static int rename_all_count = 0;
static int rename_all_done = 0;

// ---------- forward declarations ----------
static void refresh_mru_list_ui(void);
static void update_mru_highlight(void);
static void name_screen_show_list(void);
static void name_screen_show_keyboard(void);

// ---------- MRU helpers ----------
static bool is_default_name(const char *name)
{
    if (name[0] != 'P') return false;
    if (name[1] < '1' || name[1] > '9') return false;
    if (name[2] != '\0') return false;
    return true;
}

static void mru_load(void)
{
    nvs_get_name_list(mru_names);
    mru_count = 0;
    for (int i = 0; i < NAME_LIST_COUNT; i++) {
        if (mru_names[i][0] == '\0') break;
        mru_count = i + 1;
    }
}

static void mru_use_name(const char *name)
{
    int i, found = -1;

    for (i = 0; i < mru_count; i++) {
        if (strcmp(mru_names[i], name) == 0) { found = i; break; }
    }

    if (found > 0) {
        char tmp[NAME_LIST_LEN];
        memcpy(tmp, mru_names[found], NAME_LIST_LEN);
        memmove(&mru_names[1], &mru_names[0], (size_t)found * NAME_LIST_LEN);
        memcpy(mru_names[0], tmp, NAME_LIST_LEN);
    } else if (found < 0) {
        int shift = (mru_count < NAME_LIST_COUNT) ? mru_count : NAME_LIST_COUNT - 1;
        if (shift > 0)
            memmove(&mru_names[1], &mru_names[0], (size_t)shift * NAME_LIST_LEN);
        snprintf(mru_names[0], NAME_LIST_LEN, "%s", name);
        if (mru_count < NAME_LIST_COUNT) mru_count++;
    }
    /* found == 0: already at front, nothing to do */

    nvs_set_name_list((const char (*)[NAME_LIST_LEN])mru_names);
    settings_save();
}

// ---------- rename-all advance ----------
static void rename_all_advance(void)
{
    rename_all_done++;
    if (rename_all_done < rename_all_count) {
        int next = (rename_all_start + rename_all_done) % rename_all_count;
        menu_player = next;
        open_rename_screen();
    } else {
        rename_all_active = false;
        refresh_select_ui();
        refresh_damage_ui();
        back_to_main();
    }
}

// ---------- apply + return ----------
static void apply_name_and_return(const char *name)
{
    snprintf(player_names[menu_player],
             sizeof(player_names[menu_player]), "%s", name);
    if (!is_default_name(name))
        mru_use_name(name);
    refresh_multiplayer_ui();

    if (rename_all_active) {
        rename_all_advance();
    } else {
        refresh_rename_ui();
        refresh_select_ui();
        refresh_damage_ui();
        open_player_menu(menu_player);
    }
}

// ---------- events ----------
static void event_name_save(lv_event_t *e)
{
    const char *txt;

    (void)e;
    if (textarea_name == NULL) return;

    txt = lv_textarea_get_text(textarea_name);
    if (strlen(txt) == 0) {
        /* Empty: reset to default, don't add to MRU */
        snprintf(player_names[menu_player],
                 sizeof(player_names[menu_player]),
                 "P%d", menu_player + 1);
        refresh_multiplayer_ui();
        if (rename_all_active) {
            rename_all_advance();
        } else {
            refresh_rename_ui();
            refresh_select_ui();
            refresh_damage_ui();
            open_player_menu(menu_player);
        }
    } else {
        apply_name_and_return(txt);
    }
}

static void event_mru_delete(lv_event_t *e)
{
    (void)e;
    /* Only delete actual MRU entries (rows 1..mru_count) */
    if (mru_selected < 1 || mru_selected > mru_count) return;

    int del = mru_selected - 1;
    int remaining = mru_count - del - 1;
    if (remaining > 0)
        memmove(&mru_names[del], &mru_names[del + 1], (size_t)remaining * NAME_LIST_LEN);
    mru_count--;
    mru_names[mru_count][0] = '\0';

    nvs_set_name_list((const char (*)[NAME_LIST_LEN])mru_names);
    settings_save();

    /* Clamp selection */
    int total = 1 + mru_count + 1;
    if (mru_selected >= total) mru_selected = total - 1;

    refresh_mru_list_ui();
}

static void event_mru_select(lv_event_t *e)
{
    (void)e;
    if (mru_selected == 0) {
        /* Current name row — re-apply as-is */
        apply_name_and_return(player_names[menu_player]);
    } else if (mru_selected <= mru_count) {
        apply_name_and_return(mru_names[mru_selected - 1]);
    } else {
        name_screen_show_keyboard();
        refresh_rename_ui();
    }
}

static void event_mru_row_click(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx == 0) {
        /* Current name row */
        apply_name_and_return(player_names[menu_player]);
    } else if (idx <= mru_count) {
        apply_name_and_return(mru_names[idx - 1]);
    } else {
        name_screen_show_keyboard();
        refresh_rename_ui();
    }
}

// ---------- MRU list UI ----------
static void add_list_row(int idx, const char *text, lv_color_t color)
{
    lv_obj_t *row = lv_obj_create(mru_list_container);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 280, 28);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, event_mru_row_click, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
}

static void refresh_mru_list_ui(void)
{
    int i, total;

    if (mru_list_container == NULL) return;

    lv_obj_clean(mru_list_container);
    /* row 0 = current name, rows 1..mru_count = MRU, last = "Type new..." */
    total = 1 + mru_count + 1;

    add_list_row(0, player_names[menu_player], lv_color_hex(0x80CBC4));
    for (i = 0; i < mru_count; i++)
        add_list_row(1 + i, mru_names[i], lv_color_white());
    add_list_row(total - 1, "Type new...", lv_color_hex(0x7A7A7A));

    if (mru_selected >= total) mru_selected = total - 1;
    if (mru_selected < 0) mru_selected = 0;

    lv_obj_scroll_to_y(mru_list_container, 0, LV_ANIM_OFF);
    update_mru_highlight();
}

static void update_mru_highlight(void)
{
    int i;
    int total = 1 + mru_count + 1;
    uint32_t child_count = lv_obj_get_child_cnt(mru_list_container);

    for (i = 0; i < (int)child_count && i < total; i++) {
        lv_obj_t *row = lv_obj_get_child(mru_list_container, i);
        if (i == mru_selected) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        }
    }

    /* Scroll selected into view */
    if (mru_selected >= 0 && mru_selected < (int)child_count) {
        lv_obj_t *sel = lv_obj_get_child(mru_list_container, mru_selected);
        lv_coord_t sel_y = lv_obj_get_y(sel);
        lv_coord_t sel_h = lv_obj_get_height(sel);
        lv_coord_t cont_h = lv_obj_get_height(mru_list_container);
        lv_coord_t scroll_y = lv_obj_get_scroll_y(mru_list_container);

        if (sel_y - scroll_y < 0)
            lv_obj_scroll_to_y(mru_list_container, sel_y, LV_ANIM_ON);
        else if (sel_y + sel_h - scroll_y > cont_h)
            lv_obj_scroll_to_y(mru_list_container, sel_y + sel_h - cont_h, LV_ANIM_ON);
    }
}

// ---------- mode switching ----------
static void name_screen_show_list(void)
{
    name_keyboard_mode = false;
    if (mru_list_container) lv_obj_clear_flag(mru_list_container, LV_OBJ_FLAG_HIDDEN);
    if (btn_mru_select)     lv_obj_clear_flag(btn_mru_select, LV_OBJ_FLAG_HIDDEN);
    if (textarea_name)      lv_obj_add_flag(textarea_name, LV_OBJ_FLAG_HIDDEN);
    if (keyboard_name)      lv_obj_add_flag(keyboard_name, LV_OBJ_FLAG_HIDDEN);
    if (btn_name_save)      lv_obj_add_flag(btn_name_save, LV_OBJ_FLAG_HIDDEN);
}

static void name_screen_show_keyboard(void)
{
    name_keyboard_mode = true;
    if (mru_list_container) lv_obj_add_flag(mru_list_container, LV_OBJ_FLAG_HIDDEN);
    if (btn_mru_select)     lv_obj_add_flag(btn_mru_select, LV_OBJ_FLAG_HIDDEN);
    if (textarea_name)      lv_obj_clear_flag(textarea_name, LV_OBJ_FLAG_HIDDEN);
    if (keyboard_name)      lv_obj_clear_flag(keyboard_name, LV_OBJ_FLAG_HIDDEN);
    if (btn_name_save)      lv_obj_clear_flag(btn_name_save, LV_OBJ_FLAG_HIDDEN);
}

// ---------- knob / back ----------
void mru_select_next(void)
{
    if (name_keyboard_mode) return;
    if (mru_selected < mru_count + 1) {
        mru_selected++;
        update_mru_highlight();
    }
}

void mru_select_prev(void)
{
    if (name_keyboard_mode) return;
    if (mru_selected > 0) {
        mru_selected--;
        update_mru_highlight();
    }
}

bool name_screen_handle_back(void)
{
    if (name_keyboard_mode) {
        name_screen_show_list();
        refresh_rename_ui();
        return true;
    }
    if (rename_all_active) {
        rename_all_active = false;
        back_to_main();
        return true;
    }
    return false;
}

// ---------- refresh / open ----------
void refresh_rename_ui(void)
{
    char buf[40];

    if (label_name_title != NULL) {
        if (name_keyboard_mode)
            lv_label_set_text(label_name_title, "New name");
        else {
            snprintf(buf, sizeof(buf), "Rename %s", player_names[menu_player]);
            lv_label_set_text(label_name_title, buf);
        }
    }

    if (textarea_name != NULL) {
        lv_textarea_set_text(textarea_name, "");
    }

    refresh_mru_list_ui();
}

void open_rename_screen(void)
{
    mru_load();
    mru_selected = 0;
    name_screen_show_list();
    refresh_rename_ui();
    load_screen_if_needed(screen_player_name);
}

void open_rename_all_screen(void)
{
    rename_all_active = true;
    rename_all_start = menu_player;
    rename_all_count = nvs_get_num_players();
    rename_all_done = 0;
    open_rename_screen();
}

// ---------- build ----------
void build_rename_screen(void)
{
    screen_player_name = lv_obj_create(NULL);
    lv_obj_set_size(screen_player_name, 360, 360);
    lv_obj_set_style_bg_color(screen_player_name, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_player_name, 0, 0);
    lv_obj_set_scrollbar_mode(screen_player_name, LV_SCROLLBAR_MODE_OFF);

    /* Title (shared between modes) */
    label_name_title = lv_label_create(screen_player_name);
    lv_obj_set_style_text_color(label_name_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_name_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_name_title, LV_ALIGN_TOP_MID, 0, 18);

    /* --- List mode widgets --- */
    mru_list_container = lv_obj_create(screen_player_name);
    lv_obj_remove_style_all(mru_list_container);
    lv_obj_set_size(mru_list_container, 280, 210);
    lv_obj_align(mru_list_container, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_flex_flow(mru_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mru_list_container, 2, 0);
    lv_obj_set_scrollbar_mode(mru_list_container, LV_SCROLLBAR_MODE_OFF);

    btn_mru_select = lv_btn_create(screen_player_name);
    lv_obj_set_size(btn_mru_select, 120, 38);
    lv_obj_add_event_cb(btn_mru_select, event_mru_select, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_mru_select, event_mru_delete, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_t *btn_mru_label = lv_label_create(btn_mru_select);
    lv_label_set_text(btn_mru_label, "Select");
    lv_obj_center(btn_mru_label);
    lv_obj_align(btn_mru_select, LV_ALIGN_TOP_MID, 0, 278);

    /* --- Keyboard mode widgets (hidden initially) --- */
    textarea_name = lv_textarea_create(screen_player_name);
    lv_obj_set_size(textarea_name, 240, 44);
    lv_obj_align(textarea_name, LV_ALIGN_TOP_MID, 0, 56);
    lv_textarea_set_max_length(textarea_name, 15);
    lv_textarea_set_one_line(textarea_name, true);
    lv_obj_add_flag(textarea_name, LV_OBJ_FLAG_HIDDEN);

    keyboard_name = lv_keyboard_create(screen_player_name);
    lv_obj_set_size(keyboard_name, 360, 170);
    lv_obj_align(keyboard_name, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard_name, textarea_name);
    lv_obj_add_flag(keyboard_name, LV_OBJ_FLAG_HIDDEN);

    btn_name_save = make_button(screen_player_name, "save", 88, 38, event_name_save);
    lv_obj_align(btn_name_save, LV_ALIGN_TOP_MID, 0, 116);
    lv_obj_add_flag(btn_name_save, LV_OBJ_FLAG_HIDDEN);

    mru_load();
}
