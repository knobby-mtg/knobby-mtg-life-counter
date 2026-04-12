#include "ui_player_menu.h"
#include "ui_1p.h"
#include "game.h"
#include "storage.h"
#include "damage_log.h"
#include "rename.h"
#include "settings.h"

// ---------- screens ----------
lv_obj_t *screen_player_menu = NULL;
lv_obj_t *screen_player_all_damage = NULL;
lv_obj_t *screen_counter_menu = NULL;
lv_obj_t *screen_counter_edit = NULL;
lv_obj_t *screen_eliminated_player_menu = NULL;
lv_obj_t *screen_player_color_menu = NULL;
lv_obj_t *screen_player_color_picker = NULL;

// ---------- widgets ----------
static lv_obj_t *label_all_damage_title = NULL;
static lv_obj_t *label_all_damage_value = NULL;
static lv_obj_t *label_all_damage_hint = NULL;
static lv_obj_t *label_counter_edit_title = NULL;
static lv_obj_t *label_counter_edit_value = NULL;
static lv_obj_t *label_counter_edit_hint = NULL;
static lv_obj_t *label_counter_edit_icon = NULL;

// ---------- forward declarations ----------
static void open_all_damage_screen(void);
static void open_counter_edit_screen(counter_type_t type);

// ---------- refresh ----------
void refresh_all_damage_ui(void)
{
    char buf[32];

    if (label_all_damage_title != NULL) {
        lv_label_set_text(label_all_damage_title, "All players");
    }

    if (label_all_damage_value != NULL) {
        snprintf(buf, sizeof(buf), "Damage: %d", all_damage_value);
        lv_label_set_text(label_all_damage_value, buf);
    }
}

void refresh_counter_edit_ui(void)
{
    char title_buf[48];
    char value_buf[16];
    const counter_definition_t *definition = get_counter_definition(counter_edit_type);

    if (definition == NULL) return;

    if (label_counter_edit_icon != NULL) {
        const lv_font_t *icon_font = (counter_edit_type == COUNTER_TYPE_POISON)
                                      ? &mana_poison_icon_bold_16
                                      : &mana_counter_icons_16;
        lv_label_set_text(label_counter_edit_icon,
            definition->icon_text ? definition->icon_text : "");
        lv_obj_set_style_text_font(label_counter_edit_icon, icon_font, 0);
    }

    if (label_counter_edit_title != NULL) {
        snprintf(title_buf, sizeof(title_buf), "%s\n%s",
            player_names[menu_player], definition->display_name);
        lv_label_set_text(label_counter_edit_title, title_buf);
    }

    if (label_counter_edit_value != NULL) {
        snprintf(value_buf, sizeof(value_buf), "%d", counter_edit_value);
        lv_label_set_text(label_counter_edit_value, value_buf);
    }
}

// ---------- navigation ----------
void open_player_menu(int player_index)
{
    menu_player = player_index;
    load_screen_if_needed(screen_player_menu);
}

void open_counter_menu(void)
{
    load_screen_if_needed(screen_counter_menu);
}

static void open_all_damage_screen(void)
{
    all_damage_value = 0;
    refresh_all_damage_ui();
    load_screen_if_needed(screen_player_all_damage);
}

static void open_counter_edit_screen(counter_type_t type)
{
    begin_counter_edit(menu_player, type);
    refresh_counter_edit_ui();
    load_screen_if_needed(screen_counter_edit);
}

// ---------- events ----------
static void event_menu_rename(lv_event_t *e)
{
    (void)e;
    open_rename_screen();
}

static void event_menu_rename_all(lv_event_t *e)
{
    (void)e;
    open_rename_all_screen();
}

static void event_menu_cmd_damage(lv_event_t *e)
{
    (void)e;
    prepare_cmd_damage_for_player(menu_player);
    open_select_screen();
}

static void event_menu_all_damage(lv_event_t *e)
{
    (void)e;
    open_all_damage_screen();
}

static void event_menu_counters(lv_event_t *e)
{
    (void)e;
    open_counter_menu();
}

static void event_eliminated_undo(lv_event_t *e)
{
    (void)e;
    if (elimination_action_available(menu_player)) {
        undo_elimination_action(menu_player);
    } else {
        manual_uneliminate_player(menu_player);
    }
    back_to_main();
}

static void event_menu_eliminate(lv_event_t *e)
{
    (void)e;
    manual_eliminate_player(menu_player);
    back_to_main();
}

static void event_counter_commander_tax(lv_event_t *e)
{
    (void)e;
    open_counter_edit_screen(COUNTER_TYPE_COMMANDER_TAX);
}

static void event_counter_partner_tax(lv_event_t *e)
{
    (void)e;
    open_counter_edit_screen(COUNTER_TYPE_PARTNER_TAX);
}

static void event_counter_poison(lv_event_t *e)
{
    (void)e;
    open_counter_edit_screen(COUNTER_TYPE_POISON);
}

static void event_counter_experience(lv_event_t *e)
{
    (void)e;
    open_counter_edit_screen(COUNTER_TYPE_EXPERIENCE);
}

static void event_all_damage_apply(lv_event_t *e)
{
    int i;

    (void)e;
    for (i = 0; i < nvs_get_players_to_track(); i++) {
        damage_log_add(i, -all_damage_value, LOG_EVT_LIFE, -1);
        player_life[i] = clamp_life(player_life[i] - all_damage_value);
    }

    refresh_player_ui();
    back_to_main();
}

static void event_counter_apply(lv_event_t *e)
{
    (void)e;
    apply_counter_edit();
    refresh_counter_edit_ui();
    refresh_player_ui();
    back_to_main();
}

// ---------- per-player color ----------
static lv_obj_t *color_picker_swatch = NULL;
static lv_obj_t *color_picker_name_label = NULL;
static lv_obj_t *color_picker_title_label = NULL;
static int color_picker_index = 0;

static void event_menu_color(lv_event_t *e)
{
    (void)e;
    load_screen_if_needed(screen_player_color_menu);
}

static void event_color_default(lv_event_t *e)
{
    (void)e;
    player_has_override[menu_player] = false;
    player_life_color[menu_player] = false;
    refresh_player_ui();
    back_to_main();
}

static void event_color_life(lv_event_t *e)
{
    (void)e;
    player_has_override[menu_player] = true;
    player_life_color[menu_player] = true;
    refresh_player_ui();
    back_to_main();
}

static void event_color_custom(lv_event_t *e)
{
    char title_buf[32];
    (void)e;
    color_picker_index = player_color_index[menu_player];
    if (color_picker_title_label != NULL) {
        snprintf(title_buf, sizeof(title_buf), "%s\nColor", player_names[menu_player]);
        lv_label_set_text(color_picker_title_label, title_buf);
    }
    if (color_picker_swatch != NULL)
        lv_obj_set_style_bg_color(color_picker_swatch, get_custom_color_vib(color_picker_index, LIFE_VIB_MID), 0);
    if (color_picker_name_label != NULL)
        lv_label_set_text(color_picker_name_label, get_custom_color_name(color_picker_index));
    load_screen_if_needed(screen_player_color_picker);
}

void change_player_color(int delta)
{
    color_picker_index += delta;
    if (color_picker_index < 0) color_picker_index = CUSTOM_COLOR_COUNT - 1;
    if (color_picker_index >= CUSTOM_COLOR_COUNT) color_picker_index = 0;

    if (color_picker_swatch != NULL)
        lv_obj_set_style_bg_color(color_picker_swatch, get_custom_color_vib(color_picker_index, LIFE_VIB_MID), 0);
    if (color_picker_name_label != NULL)
        lv_label_set_text(color_picker_name_label, get_custom_color_name(color_picker_index));
}

void commit_player_color(void)
{
    player_has_override[menu_player] = true;
    player_color_index[menu_player] = color_picker_index;
    player_life_color[menu_player] = false;
    refresh_player_ui();
}

static void event_color_apply(lv_event_t *e)
{
    (void)e;
    commit_player_color();
    back_to_main();
}

// ---------- screen builders ----------
void build_player_menu_screen(void)
{
    quad_item_t items[4] = {
        {"Name/\nColor", event_menu_color,     true,  LV_EVENT_CLICKED},
        {"Commander\nDamage", event_menu_cmd_damage, true,  LV_EVENT_CLICKED},
        {"All\nDamage", event_menu_all_damage, true,  LV_EVENT_CLICKED},
        {"Counters",    event_menu_counters,   true,  LV_EVENT_SHORT_CLICKED},
    };
    build_quad_screen(&screen_player_menu, items);

    /* Long-press Counters to manually eliminate */
    lv_obj_t *counters_btn = lv_obj_get_child(screen_player_menu, 3);
    lv_obj_add_event_cb(counters_btn, event_menu_eliminate, LV_EVENT_LONG_PRESSED, NULL);
}

void build_eliminated_player_menu_screen(void)
{
    screen_eliminated_player_menu = lv_obj_create(NULL);
    lv_obj_set_size(screen_eliminated_player_menu, 360, 360);
    lv_obj_set_style_bg_color(screen_eliminated_player_menu, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_eliminated_player_menu, 0, 0);
    lv_obj_set_scrollbar_mode(screen_eliminated_player_menu, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen_eliminated_player_menu);
    lv_label_set_text(title, "Undo elimination");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *hint = lv_label_create(screen_eliminated_player_menu);
    lv_label_set_text(hint, "Restore the action that eliminated this player");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn = make_button(screen_eliminated_player_menu, "Undo", 120, 46, event_eliminated_undo);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

void build_counter_menu_screen(void)
{
    int i;
    static const counter_type_t types[4] = {
        COUNTER_TYPE_COMMANDER_TAX,
        COUNTER_TYPE_PARTNER_TAX,
        COUNTER_TYPE_POISON,
        COUNTER_TYPE_EXPERIENCE,
    };
    lv_event_cb_t const cbs[4] = {
        event_counter_commander_tax,
        event_counter_partner_tax,
        event_counter_poison,
        event_counter_experience,
    };
    quad_item_t items[4];

    for (i = 0; i < 4; i++) {
        const counter_definition_t *def = get_counter_definition(types[i]);
        items[i].label      = def->menu_label;
        items[i].cb         = cbs[i];
        items[i].enabled    = true;
        items[i].event      = LV_EVENT_CLICKED;
        items[i].icon       = def->icon_text;
        items[i].icon_font  = (types[i] == COUNTER_TYPE_POISON)
                              ? &mana_poison_icon_bold_16
                              : &mana_counter_icons_16;
    }

    build_quad_screen(&screen_counter_menu, items);
}

void build_all_damage_screen(void)
{
    screen_player_all_damage = lv_obj_create(NULL);
    lv_obj_set_size(screen_player_all_damage, 360, 360);
    lv_obj_set_style_bg_color(screen_player_all_damage, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_player_all_damage, 0, 0);
    lv_obj_set_scrollbar_mode(screen_player_all_damage, LV_SCROLLBAR_MODE_OFF);

    label_all_damage_title = lv_label_create(screen_player_all_damage);
    lv_obj_set_style_text_color(label_all_damage_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_all_damage_title, &lv_font_montserrat_22, 0);
    lv_obj_align(label_all_damage_title, LV_ALIGN_TOP_MID, 0, 26);

    label_all_damage_value = lv_label_create(screen_player_all_damage);
    lv_obj_set_style_text_color(label_all_damage_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_all_damage_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_all_damage_value, LV_ALIGN_CENTER, 0, -8);

    label_all_damage_hint = lv_label_create(screen_player_all_damage);
    lv_label_set_text(label_all_damage_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_all_damage_hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_all_damage_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_all_damage_hint, LV_ALIGN_CENTER, 0, 38);

    lv_obj_t *btn = make_button(screen_player_all_damage, "Apply", 120, 46, event_all_damage_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

void build_counter_edit_screen(void)
{
    lv_obj_t *btn;

    screen_counter_edit = lv_obj_create(NULL);
    lv_obj_set_size(screen_counter_edit, 360, 360);
    lv_obj_set_style_bg_color(screen_counter_edit, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_counter_edit, 0, 0);
    lv_obj_set_scrollbar_mode(screen_counter_edit, LV_SCROLLBAR_MODE_OFF);

    label_counter_edit_icon = lv_label_create(screen_counter_edit);
    lv_label_set_text(label_counter_edit_icon, "");
    lv_obj_set_style_text_color(label_counter_edit_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_counter_edit_icon, &mana_counter_icons_16, 0);
    lv_obj_align(label_counter_edit_icon, LV_ALIGN_TOP_MID, 0, 12);

    label_counter_edit_title = lv_label_create(screen_counter_edit);
    lv_label_set_text(label_counter_edit_title, "P1\nCommander Tax");
    lv_obj_set_style_text_color(label_counter_edit_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_counter_edit_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(label_counter_edit_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_counter_edit_title, LV_ALIGN_TOP_MID, 0, 42);

    label_counter_edit_value = lv_label_create(screen_counter_edit);
    lv_label_set_text(label_counter_edit_value, "0");
    lv_obj_set_style_text_color(label_counter_edit_value, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_counter_edit_value, &lv_font_montserrat_32, 0);
    lv_obj_align(label_counter_edit_value, LV_ALIGN_CENTER, 0, -4);

    label_counter_edit_hint = lv_label_create(screen_counter_edit);
    lv_label_set_text(label_counter_edit_hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(label_counter_edit_hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(label_counter_edit_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_counter_edit_hint, LV_ALIGN_CENTER, 0, 34);

    btn = make_button(screen_counter_edit, "Apply", 120, 46, event_counter_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}

// ---------- per-player color screens ----------
void build_player_color_menu_screen(void)
{
    quad_item_t items[4] = {
        {"Rename",          event_menu_rename,   true, LV_EVENT_SHORT_CLICKED},
        {"Default\nSetting", event_color_default, true, LV_EVENT_CLICKED},
        {"Life\nColor",     event_color_life,    true, LV_EVENT_CLICKED},
        {"Custom\nColor",   event_color_custom,  true, LV_EVENT_CLICKED},
    };
    build_quad_screen(&screen_player_color_menu, items);

    /* Long-press Rename to rename all players sequentially */
    lv_obj_t *name_btn = lv_obj_get_child(screen_player_color_menu, 0);
    lv_obj_add_event_cb(name_btn, event_menu_rename_all, LV_EVENT_LONG_PRESSED, NULL);
}

void build_player_color_picker_screen(void)
{
    lv_obj_t *hint;

    screen_player_color_picker = lv_obj_create(NULL);
    lv_obj_set_size(screen_player_color_picker, 360, 360);
    lv_obj_set_style_bg_color(screen_player_color_picker, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_player_color_picker, 0, 0);
    lv_obj_set_scrollbar_mode(screen_player_color_picker, LV_SCROLLBAR_MODE_OFF);

    color_picker_title_label = lv_label_create(screen_player_color_picker);
    lv_label_set_text(color_picker_title_label, "Color");
    lv_obj_set_style_text_color(color_picker_title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(color_picker_title_label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(color_picker_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(color_picker_title_label, LV_ALIGN_TOP_MID, 0, 20);

    color_picker_swatch = lv_obj_create(screen_player_color_picker);
    lv_obj_remove_style_all(color_picker_swatch);
    lv_obj_set_size(color_picker_swatch, 120, 120);
    lv_obj_align(color_picker_swatch, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_radius(color_picker_swatch, 12, 0);
    lv_obj_set_style_bg_opa(color_picker_swatch, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(color_picker_swatch, get_custom_color_vib(0, LIFE_VIB_MID), 0);
    lv_obj_clear_flag(color_picker_swatch, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    color_picker_name_label = lv_label_create(screen_player_color_picker);
    lv_label_set_text(color_picker_name_label, get_custom_color_name(0));
    lv_obj_set_style_text_color(color_picker_name_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(color_picker_name_label, &lv_font_montserrat_16, 0);
    lv_obj_align(color_picker_name_label, LV_ALIGN_CENTER, 0, 54);

    hint = lv_label_create(screen_player_color_picker);
    lv_label_set_text(hint, "Turn knob, then apply");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7A7A7A), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 73);

    lv_obj_t *btn = make_button(screen_player_color_picker, "Apply", 120, 46, event_color_apply);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
}
