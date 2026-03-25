#ifndef _SCR_ST77916_H_
#define _SCR_ST77916_H_

#include "pincfg.h"
#include <lvgl.h>
#include <ESP_IOExpander_Library.h>
#include <ESP_Panel_Library.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
// #include "bidi_switch_knob.h"
#include "knob.h"

#define SCREEN_RES_HOR 360
#define SCREEN_RES_VER 360
#define DEFAULT_UI_BRIGHTNESS_PERCENT 30

#define EXAMPLE_TOUCH_I2C_SCL_PULLUP    (1)  // 0/1
#define EXAMPLE_TOUCH_I2C_SDA_PULLUP    (1)  // 0/1

#define CONFIG_KNOB_HIGH_LIMIT     1
#define CONFIG_KNOB_LOW_LIMIT      1

static const char *SCR_TAG = "scr_st77916";

static lv_color_t *disp_draw_buf;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp_handle = NULL;
static lv_indev_t *indev_touchpad;
static bool disp_flush_uses_callback = false;
static ESP_PanelBacklightPWM_LEDC *backlight = NULL;
static ESP_PanelLcd *lcd = NULL;
static ESP_PanelTouch *touch = NULL;
static knob_handle_t knob_handle = NULL;
#define USE_CUSTOM_INIT_CMD 0 // 是否用自定义的初始化代码

#if TOUCH_PIN_NUM_INT >= 0

IRAM_ATTR bool onTouchInterruptCallback(void *user_data)
{
  return false;
}

#endif

const esp_lcd_panel_vendor_init_cmd_t lcd_init_cmd[] = {
     {0xF0, (uint8_t[]){0x28}, 1, 0},
    {0xF2, (uint8_t[]){0x28}, 1, 0},
    {0x73, (uint8_t[]){0xF0}, 1, 0},
    {0x7C, (uint8_t[]){0xD1}, 1, 0},
    {0x83, (uint8_t[]){0xE0}, 1, 0},
    {0x84, (uint8_t[]){0x61}, 1, 0},
    {0xF2, (uint8_t[]){0x82}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0xF0, (uint8_t[]){0x01}, 1, 0},
    {0xF1, (uint8_t[]){0x01}, 1, 0},
    {0xB0, (uint8_t[]){0x56}, 1, 0},
    {0xB1, (uint8_t[]){0x4D}, 1, 0},
    {0xB2, (uint8_t[]){0x24}, 1, 0},
    {0xB4, (uint8_t[]){0x87}, 1, 0},
    {0xB5, (uint8_t[]){0x44}, 1, 0},
    {0xB6, (uint8_t[]){0x8B}, 1, 0},
    {0xB7, (uint8_t[]){0x40}, 1, 0},
    {0xB8, (uint8_t[]){0x86}, 1, 0},
    {0xBA, (uint8_t[]){0x00}, 1, 0},
    {0xBB, (uint8_t[]){0x08}, 1, 0},
    {0xBC, (uint8_t[]){0x08}, 1, 0},
    {0xBD, (uint8_t[]){0x00}, 1, 0},
    {0xC0, (uint8_t[]){0x80}, 1, 0},
    {0xC1, (uint8_t[]){0x10}, 1, 0},
    {0xC2, (uint8_t[]){0x37}, 1, 0},
    {0xC3, (uint8_t[]){0x80}, 1, 0},
    {0xC4, (uint8_t[]){0x10}, 1, 0},
    {0xC5, (uint8_t[]){0x37}, 1, 0},
    {0xC6, (uint8_t[]){0xA9}, 1, 0},
    {0xC7, (uint8_t[]){0x41}, 1, 0},
    {0xC8, (uint8_t[]){0x01}, 1, 0},
    {0xC9, (uint8_t[]){0xA9}, 1, 0},
    {0xCA, (uint8_t[]){0x41}, 1, 0},
    {0xCB, (uint8_t[]){0x01}, 1, 0},
    {0xD0, (uint8_t[]){0x91}, 1, 0},
    {0xD1, (uint8_t[]){0x68}, 1, 0},
    {0xD2, (uint8_t[]){0x68}, 1, 0},
    {0xF5, (uint8_t[]){0x00, 0xA5}, 2, 0},
    {0xDD, (uint8_t[]){0x4F}, 1, 0},
    {0xDE, (uint8_t[]){0x4F}, 1, 0},
    {0xF1, (uint8_t[]){0x10}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0xF0, (uint8_t[]){0x02}, 1, 0},
    {0xE0, (uint8_t[]){0xF0, 0x0A, 0x10, 0x09, 0x09, 0x36, 0x35, 0x33, 0x4A, 0x29, 0x15, 0x15, 0x2E, 0x34}, 14, 0},
    {0xE1, (uint8_t[]){0xF0, 0x0A, 0x0F, 0x08, 0x08, 0x05, 0x34, 0x33, 0x4A, 0x39, 0x15, 0x15, 0x2D, 0x33}, 14, 0},
    {0xF0, (uint8_t[]){0x10}, 1, 0},
    {0xF3, (uint8_t[]){0x10}, 1, 0},
    {0xE0, (uint8_t[]){0x07}, 1, 0},
    {0xE1, (uint8_t[]){0x00}, 1, 0},
    {0xE2, (uint8_t[]){0x00}, 1, 0},
    {0xE3, (uint8_t[]){0x00}, 1, 0},
    {0xE4, (uint8_t[]){0xE0}, 1, 0},
    {0xE5, (uint8_t[]){0x06}, 1, 0},
    {0xE6, (uint8_t[]){0x21}, 1, 0},
    {0xE7, (uint8_t[]){0x01}, 1, 0},
    {0xE8, (uint8_t[]){0x05}, 1, 0},
    {0xE9, (uint8_t[]){0x02}, 1, 0},
    {0xEA, (uint8_t[]){0xDA}, 1, 0},
    {0xEB, (uint8_t[]){0x00}, 1, 0},
    {0xEC, (uint8_t[]){0x00}, 1, 0},
    {0xED, (uint8_t[]){0x0F}, 1, 0},
    {0xEE, (uint8_t[]){0x00}, 1, 0},
    {0xEF, (uint8_t[]){0x00}, 1, 0},
    {0xF8, (uint8_t[]){0x00}, 1, 0},
    {0xF9, (uint8_t[]){0x00}, 1, 0},
    {0xFA, (uint8_t[]){0x00}, 1, 0},
    {0xFB, (uint8_t[]){0x00}, 1, 0},
    {0xFC, (uint8_t[]){0x00}, 1, 0},
    {0xFD, (uint8_t[]){0x00}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xFF, (uint8_t[]){0x00}, 1, 0},
    {0x60, (uint8_t[]){0x40}, 1, 0},
    {0x61, (uint8_t[]){0x04}, 1, 0},
    {0x62, (uint8_t[]){0x00}, 1, 0},
    {0x63, (uint8_t[]){0x42}, 1, 0},
    {0x64, (uint8_t[]){0xD9}, 1, 0},
    {0x65, (uint8_t[]){0x00}, 1, 0},
    {0x66, (uint8_t[]){0x00}, 1, 0},
    {0x67, (uint8_t[]){0x00}, 1, 0},
    {0x68, (uint8_t[]){0x00}, 1, 0},
    {0x69, (uint8_t[]){0x00}, 1, 0},
    {0x6A, (uint8_t[]){0x00}, 1, 0},
    {0x6B, (uint8_t[]){0x00}, 1, 0},
    {0x70, (uint8_t[]){0x40}, 1, 0},
    {0x71, (uint8_t[]){0x03}, 1, 0},
    {0x72, (uint8_t[]){0x00}, 1, 0},
    {0x73, (uint8_t[]){0x42}, 1, 0},
    {0x74, (uint8_t[]){0xD8}, 1, 0},
    {0x75, (uint8_t[]){0x00}, 1, 0},
    {0x76, (uint8_t[]){0x00}, 1, 0},
    {0x77, (uint8_t[]){0x00}, 1, 0},
    {0x78, (uint8_t[]){0x00}, 1, 0},
    {0x79, (uint8_t[]){0x00}, 1, 0},
    {0x7A, (uint8_t[]){0x00}, 1, 0},
    {0x7B, (uint8_t[]){0x00}, 1, 0},
    {0x80, (uint8_t[]){0x48}, 1, 0},
    {0x81, (uint8_t[]){0x00}, 1, 0},
    {0x82, (uint8_t[]){0x06}, 1, 0},
    {0x83, (uint8_t[]){0x02}, 1, 0},
    {0x84, (uint8_t[]){0xD6}, 1, 0},
    {0x85, (uint8_t[]){0x04}, 1, 0},
    {0x86, (uint8_t[]){0x00}, 1, 0},
    {0x87, (uint8_t[]){0x00}, 1, 0},
    {0x88, (uint8_t[]){0x48}, 1, 0},
    {0x89, (uint8_t[]){0x00}, 1, 0},
    {0x8A, (uint8_t[]){0x08}, 1, 0},
    {0x8B, (uint8_t[]){0x02}, 1, 0},
    {0x8C, (uint8_t[]){0xD8}, 1, 0},
    {0x8D, (uint8_t[]){0x04}, 1, 0},
    {0x8E, (uint8_t[]){0x00}, 1, 0},
    {0x8F, (uint8_t[]){0x00}, 1, 0},
    {0x90, (uint8_t[]){0x48}, 1, 0},
    {0x91, (uint8_t[]){0x00}, 1, 0},
    {0x92, (uint8_t[]){0x0A}, 1, 0},
    {0x93, (uint8_t[]){0x02}, 1, 0},
    {0x94, (uint8_t[]){0xDA}, 1, 0},
    {0x95, (uint8_t[]){0x04}, 1, 0},
    {0x96, (uint8_t[]){0x00}, 1, 0},
    {0x97, (uint8_t[]){0x00}, 1, 0},
    {0x98, (uint8_t[]){0x48}, 1, 0},
    {0x99, (uint8_t[]){0x00}, 1, 0},
    {0x9A, (uint8_t[]){0x0C}, 1, 0},
    {0x9B, (uint8_t[]){0x02}, 1, 0},
    {0x9C, (uint8_t[]){0xDC}, 1, 0},
    {0x9D, (uint8_t[]){0x04}, 1, 0},
    {0x9E, (uint8_t[]){0x00}, 1, 0},
    {0x9F, (uint8_t[]){0x00}, 1, 0},
    {0xA0, (uint8_t[]){0x48}, 1, 0},
    {0xA1, (uint8_t[]){0x00}, 1, 0},
    {0xA2, (uint8_t[]){0x05}, 1, 0},
    {0xA3, (uint8_t[]){0x02}, 1, 0},
    {0xA4, (uint8_t[]){0xD5}, 1, 0},
    {0xA5, (uint8_t[]){0x04}, 1, 0},
    {0xA6, (uint8_t[]){0x00}, 1, 0},
    {0xA7, (uint8_t[]){0x00}, 1, 0},
    {0xA8, (uint8_t[]){0x48}, 1, 0},
    {0xA9, (uint8_t[]){0x00}, 1, 0},
    {0xAA, (uint8_t[]){0x07}, 1, 0},
    {0xAB, (uint8_t[]){0x02}, 1, 0},
    {0xAC, (uint8_t[]){0xD7}, 1, 0},
    {0xAD, (uint8_t[]){0x04}, 1, 0},
    {0xAE, (uint8_t[]){0x00}, 1, 0},
    {0xAF, (uint8_t[]){0x00}, 1, 0},
    {0xB0, (uint8_t[]){0x48}, 1, 0},
    {0xB1, (uint8_t[]){0x00}, 1, 0},
    {0xB2, (uint8_t[]){0x09}, 1, 0},
    {0xB3, (uint8_t[]){0x02}, 1, 0},
    {0xB4, (uint8_t[]){0xD9}, 1, 0},
    {0xB5, (uint8_t[]){0x04}, 1, 0},
    {0xB6, (uint8_t[]){0x00}, 1, 0},
    {0xB7, (uint8_t[]){0x00}, 1, 0},
    {0xB8, (uint8_t[]){0x48}, 1, 0},
    {0xB9, (uint8_t[]){0x00}, 1, 0},
    {0xBA, (uint8_t[]){0x0B}, 1, 0},
    {0xBB, (uint8_t[]){0x02}, 1, 0},
    {0xBC, (uint8_t[]){0xDB}, 1, 0},
    {0xBD, (uint8_t[]){0x04}, 1, 0},
    {0xBE, (uint8_t[]){0x00}, 1, 0},
    {0xBF, (uint8_t[]){0x00}, 1, 0},
    {0xC0, (uint8_t[]){0x10}, 1, 0},
    {0xC1, (uint8_t[]){0x47}, 1, 0},
    {0xC2, (uint8_t[]){0x56}, 1, 0},
    {0xC3, (uint8_t[]){0x65}, 1, 0},
    {0xC4, (uint8_t[]){0x74}, 1, 0},
    {0xC5, (uint8_t[]){0x88}, 1, 0},
    {0xC6, (uint8_t[]){0x99}, 1, 0},
    {0xC7, (uint8_t[]){0x01}, 1, 0},
    {0xC8, (uint8_t[]){0xBB}, 1, 0},
    {0xC9, (uint8_t[]){0xAA}, 1, 0},
    {0xD0, (uint8_t[]){0x10}, 1, 0},
    {0xD1, (uint8_t[]){0x47}, 1, 0},
    {0xD2, (uint8_t[]){0x56}, 1, 0},
    {0xD3, (uint8_t[]){0x65}, 1, 0},
    {0xD4, (uint8_t[]){0x74}, 1, 0},
    {0xD5, (uint8_t[]){0x88}, 1, 0},
    {0xD6, (uint8_t[]){0x99}, 1, 0},
    {0xD7, (uint8_t[]){0x01}, 1, 0},
    {0xD8, (uint8_t[]){0xBB}, 1, 0},
    {0xD9, (uint8_t[]){0xAA}, 1, 0},
    {0xF3, (uint8_t[]){0x01}, 1, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0x21, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x29, (uint8_t[]){0x00}, 1, 0}
};

#define TFT_SPI_FREQ_HZ (50 * 1000 * 1000)

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  ESP_PanelLcd *lcd = (ESP_PanelLcd *)disp->user_data;

  if (disp == NULL || area == NULL || color_p == NULL || lcd == NULL) {
    if (disp != NULL) {
      lv_disp_flush_ready(disp);
    }
    return;
  }

  const int offsetx1 = area->x1;
  const int offsetx2 = area->x2;
  const int offsety1 = area->y1;
  const int offsety2 = area->y2;
  lcd->drawBitmap(offsetx1, offsety1, offsetx2 - offsetx1 + 1, offsety2 - offsety1 + 1, (const uint8_t *)color_p);

  if (!disp_flush_uses_callback) {
    lv_disp_flush_ready(disp);
  }
}

IRAM_ATTR bool onRefreshFinishCallback(void *user_data)
{
  lv_disp_drv_t *drv = (lv_disp_drv_t *)user_data;
  if (drv == NULL) {
    return false;
  }
  lv_disp_flush_ready(drv);
  return false;
}

void setRotation(uint8_t rot)
{
  if (rot > 3)
    return;
  if (lcd == NULL || touch == NULL)
    return;

  switch (rot)
  {
  case 1: // 顺时针90度
    lcd->swapXY(true);
    lcd->mirrorX(true);
    lcd->mirrorY(false);
    touch->swapXY(true);
    touch->mirrorX(true);
    touch->mirrorY(false);
    break;
  case 2:
    lcd->swapXY(false);
    lcd->mirrorX(true);
    lcd->mirrorY(true);
    touch->swapXY(false);
    touch->mirrorX(true);
    touch->mirrorY(true);
    break;
  case 3:
    lcd->swapXY(true);
    lcd->mirrorX(false);
    lcd->mirrorY(true);
    touch->swapXY(true);
    touch->mirrorX(false);
    touch->mirrorY(true);
    break;
  default:
    lcd->swapXY(false);
    lcd->mirrorX(false);
    lcd->mirrorY(false);
    touch->swapXY(false);
    touch->mirrorX(false);
    touch->mirrorY(false);
    break;
  }
}

void screen_switch(bool on)
{
  if (NULL == backlight)
    return;
  if (on)
    backlight->on();
  else
    backlight->off();
}

// 输入值为0-100
void set_brightness(uint8_t bri)
{
  if (NULL == backlight)
    return;
  backlight->setBrightness(bri);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
  ESP_PanelTouch *tp = (ESP_PanelTouch *)indev_drv->user_data;
  ESP_PanelTouchPoint point;

  int read_touch_result = tp->readPoints(&point, 1);
  if (read_touch_result > 0)
  {
    data->point.x = point.x;
    data->point.y = point.y;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void _knob_right_cb(void *arg, void *data)
{
    knob_handle_t knob = (knob_handle_t)arg;
    (void)data;
    knob_change(KNOB_RIGHT, iot_knob_get_count_value(knob));
}

static void _knob_left_cb(void *arg, void *data)
{
    knob_handle_t knob = (knob_handle_t)arg;
    (void)data;
    knob_change(KNOB_LEFT, iot_knob_get_count_value(knob));
}

static lv_indev_t *indev_init(ESP_PanelTouch *tp)
{
  if (tp == NULL || tp->getHandle() == nullptr) {
    ESP_LOGE(SCR_TAG, "Touch device is not initialized");
    return NULL;
  }

  static lv_indev_drv_t indev_drv_tp;
  lv_indev_drv_init(&indev_drv_tp);
  indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
  indev_drv_tp.read_cb = touchpad_read;
  indev_drv_tp.user_data = (void *)tp;
  return lv_indev_drv_register(&indev_drv_tp);
}

static void scr_cleanup_failed_init(lv_disp_t *disp,
                                    lv_indev_t *touch_indev,
                                    knob_handle_t knob_handle_local,
                                    lv_color_t *draw_buf_local,
                                    ESP_PanelTouch *touch_local,
                                    ESP_PanelLcd *lcd_local,
                                    ESP_PanelBacklightPWM_LEDC *backlight_local)
{
  if (touch_indev != NULL) {
    lv_indev_delete(touch_indev);
  }

  if (knob_handle_local != NULL) {
    iot_knob_delete(knob_handle_local);
  }

  if (disp != NULL) {
    lv_disp_remove(disp);
  }

  if (draw_buf_local != NULL) {
    heap_caps_free(draw_buf_local);
  }

  if (backlight_local != NULL) {
    backlight_local->off();
    delete backlight_local;
  }

  if (touch_local != NULL) {
    delete touch_local;
  }

  if (lcd_local != NULL) {
    delete lcd_local;
  }
}

bool scr_lvgl_init()
{
  lv_disp_t *disp = NULL;
  lv_indev_t *new_indev_touchpad = NULL;
  lv_color_t *new_disp_draw_buf = NULL;
  knob_handle_t new_knob_handle = NULL;
  ESP_PanelBacklightPWM_LEDC *new_backlight = NULL;
  ESP_PanelLcd *new_lcd = NULL;
  ESP_PanelTouch *new_touch = NULL;
  ESP_PanelBusI2C *touch_bus = NULL;
  ESP_PanelBusQSPI *panel_bus = NULL;
  knob_config_t cfg = {0};
  size_t lv_cache_rows = 72;
  size_t draw_buffer_size;
  bool ok = false;

  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_13_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 5000,
      .clk_cfg = LEDC_AUTO_CLK};
  if (ledc_timer_config(&ledc_timer) != ESP_OK) {
    ESP_LOGE(SCR_TAG, "Backlight LEDC timer configuration failed");
    return false;
  }

  ledc_channel_config_t ledc_channel = {
      .gpio_num = (TFT_BLK),
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0};

  if (ledc_channel_config(&ledc_channel) != ESP_OK) {
    ESP_LOGE(SCR_TAG, "Backlight LEDC channel configuration failed");
    return false;
  }

  new_backlight = new ESP_PanelBacklightPWM_LEDC(TFT_BLK, 1);
  if (new_backlight == NULL) {
    ESP_LOGE(SCR_TAG, "Backlight allocation failed");
    return false;
  }
  new_backlight->begin();
  new_backlight->off();

  touch_bus = new ESP_PanelBusI2C(TOUCH_PIN_NUM_I2C_SCL, TOUCH_PIN_NUM_I2C_SDA, ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG());
  if (touch_bus == NULL) {
    ESP_LOGE(SCR_TAG, "Touch bus allocation failed");
    goto cleanup;
  }
  // touch_bus->configI2C_Address(0x15);
  touch_bus->configI2cFreqHz(400000);
  // touch_bus->configI2C_PullupEnable(EXAMPLE_TOUCH_I2C_SDA_PULLUP, EXAMPLE_TOUCH_I2C_SCL_PULLUP);
  
  if (!touch_bus->begin()) {
    ESP_LOGE(SCR_TAG, "Touch bus begin failed");
    goto cleanup;
  }

  new_touch = new ESP_PanelTouch_CST816S(touch_bus, SCREEN_RES_HOR, SCREEN_RES_VER, TOUCH_PIN_NUM_RST, TOUCH_PIN_NUM_INT);
  if (new_touch == NULL) {
    ESP_LOGE(SCR_TAG, "Touch allocation failed");
    goto cleanup;
  }

  new_touch->init();
  new_touch->begin();
  if (new_touch->getHandle() == nullptr) {
    ESP_LOGE(SCR_TAG, "Touch initialization failed");
    goto cleanup;
  }

#if TOUCH_PIN_NUM_INT >= 0
  new_touch->attachInterruptCallback(onTouchInterruptCallback, NULL);
#endif

  panel_bus = new ESP_PanelBusQSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
  if (panel_bus == NULL) {
    ESP_LOGE(SCR_TAG, "Display bus allocation failed");
    goto cleanup;
  }
  panel_bus->configQspiFreqHz(TFT_SPI_FREQ_HZ);
  if (!panel_bus->begin()) {
    ESP_LOGE(SCR_TAG, "Display bus begin failed");
    goto cleanup;
  }

  new_lcd = new ESP_PanelLcd_ST77916(panel_bus, 16, TFT_RST);
  if (new_lcd == NULL) {
    ESP_LOGE(SCR_TAG, "Display allocation failed");
    goto cleanup;
  }
  // 注意，初始化代码的设置必须在INIT之前
  new_lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0]));
  new_lcd->init();
  new_lcd->reset();
  new_lcd->begin();

  new_lcd->invertColor(true);
  // setRotation(0);  //设置屏幕方向
  new_lcd->displayOn();

  new_backlight->setBrightness(DEFAULT_UI_BRIGHTNESS_PERCENT);
  new_backlight->on();

  lv_init();
  draw_buffer_size = lv_cache_rows * SCREEN_RES_HOR * sizeof(lv_color_t);
  new_disp_draw_buf = (lv_color_t *)heap_caps_malloc(draw_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (new_disp_draw_buf == NULL) {
    ESP_LOGE(SCR_TAG, "LVGL draw buffer allocation failed (%u bytes)", (unsigned int)draw_buffer_size);
    goto cleanup;
  }

  lv_disp_draw_buf_init(&draw_buf, new_disp_draw_buf, NULL, SCREEN_RES_HOR * lv_cache_rows);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_RES_HOR;
  disp_drv.ver_res = SCREEN_RES_VER;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.user_data = (void *)new_lcd;
  disp = lv_disp_drv_register(&disp_drv);
  if (disp == NULL) {
    ESP_LOGE(SCR_TAG, "LVGL display driver registration failed");
    goto cleanup;
  }

  disp_flush_uses_callback = false;
  if (new_lcd->getBus()->getType() != ESP_PANEL_BUS_TYPE_RGB)
  {
    // lcd->attachRefreshFinishCallback(onRefreshFinishCallback, (void *)disp->driver);attachDrawBitmapFinishCallback
    new_lcd->attachDrawBitmapFinishCallback(onRefreshFinishCallback, (void *)disp->driver);
    disp_flush_uses_callback = true;
  }
  new_indev_touchpad = indev_init(new_touch);
  if (new_indev_touchpad == NULL) {
    ESP_LOGE(SCR_TAG, "Touch input registration failed");
    goto cleanup;
  }

    cfg.gpio_encoder_a = ROTARY_ENC_PIN_A;
    cfg.gpio_encoder_b = ROTARY_ENC_PIN_B;
  new_knob_handle = iot_knob_create(&cfg);
  if (new_knob_handle == NULL)
    {
        ESP_LOGE(SCR_TAG, "Knob create failed");
        goto cleanup;
    }
  if (iot_knob_register_cb(new_knob_handle, KNOB_LEFT, _knob_left_cb, NULL) != ESP_OK) {
    ESP_LOGE(SCR_TAG, "Knob left callback registration failed");
    goto cleanup;
  }
  if (iot_knob_register_cb(new_knob_handle, KNOB_RIGHT, _knob_right_cb, NULL) != ESP_OK) {
    ESP_LOGE(SCR_TAG, "Knob right callback registration failed");
    goto cleanup;
  }

  backlight = new_backlight;
  lcd = new_lcd;
  touch = new_touch;
  disp_draw_buf = new_disp_draw_buf;
  disp_handle = disp;
  indev_touchpad = new_indev_touchpad;
  knob_handle = new_knob_handle;
  ok = true;

cleanup:
  if (!ok) {
    scr_cleanup_failed_init(disp,
                            new_indev_touchpad,
                            new_knob_handle,
                            new_disp_draw_buf,
                            new_touch,
                            new_lcd,
                            new_backlight);
    if (touch_bus != NULL && new_touch == NULL) {
      delete touch_bus;
    }
    if (panel_bus != NULL && new_lcd == NULL) {
      delete panel_bus;
    }
    disp_flush_uses_callback = false;
  }

  return ok;
}

#endif