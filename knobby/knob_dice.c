#include "knob_dice.h"
#include "knob_life.h"
#include "esp_random.h"

// ---------- state ----------
lv_obj_t *screen_dice = NULL;
static lv_obj_t *label_dice_result = NULL;
static lv_obj_t *label_dice_hint = NULL;

// ---------- refresh ----------
void refresh_dice_ui(void)
{
    char buf[8];

    if (label_dice_result == NULL) return;

    if (dice_result <= 0) {
        lv_label_set_text(label_dice_result, "--");
    } else {
        snprintf(buf, sizeof(buf), "%d", dice_result);
        lv_label_set_text(label_dice_result, buf);
    }
}

// ---------- open ----------
void open_dice_screen(void)
{
    lv_anim_t anim;

    refresh_dice_ui();
    load_screen_if_needed(screen_dice);

    if (label_dice_result != NULL) {
        lv_obj_set_y(label_dice_result, -10);
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, label_dice_result);
        lv_anim_set_values(&anim, -10, -24);
        lv_anim_set_time(&anim, 120);
        lv_anim_set_playback_time(&anim, 140);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_start(&anim);
    }
}

// ---------- events ----------
static void event_dice_tap(lv_event_t *e)
{
    (void)e;
    dice_result = (int)(esp_random() % 20U) + 1;
    open_dice_screen();
}

void event_tool_dice(lv_event_t *e)
{
    (void)e;
    dice_result = (int)(esp_random() % 20U) + 1;
    open_dice_screen();
}

// ---------- build ----------
void build_dice_screen(void)
{
    screen_dice = lv_obj_create(NULL);
    lv_obj_set_size(screen_dice, 360, 360);
    lv_obj_set_style_bg_color(screen_dice, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_dice, 0, 0);
    lv_obj_set_scrollbar_mode(screen_dice, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(screen_dice, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen_dice, event_dice_tap, LV_EVENT_LONG_PRESSED, NULL);

    label_dice_result = lv_label_create(screen_dice);
    lv_label_set_text(label_dice_result, "--");
    lv_obj_set_style_text_color(label_dice_result, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_dice_result, &lv_font_montserrat_32, 0);
    lv_obj_align(label_dice_result, LV_ALIGN_CENTER, 0, -10);

    label_dice_hint = lv_label_create(screen_dice);
    lv_label_set_text(label_dice_hint, "long press to re-roll");
    lv_obj_set_style_text_color(label_dice_hint, lv_color_hex(0x8A8A8A), 0);
    lv_obj_set_style_text_font(label_dice_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(label_dice_hint, LV_ALIGN_CENTER, 0, 42);
}
