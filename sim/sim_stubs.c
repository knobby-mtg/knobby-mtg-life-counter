#include "sim_stubs.h"
#include "board_detect.h"
#include "bidi_switch_knob.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/ledc.h"
#include "esp_random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- sim_millis ---- */

static uint32_t sim_tick_ms = 0;

uint32_t sim_millis(void)
{
    return sim_tick_ms;
}

void sim_tick_advance(uint32_t ms)
{
    sim_tick_ms += ms;
}

/* ---- Board detection ---- */

const board_pins_t board_k518 = {
    .name       = "JC3636K518",
    .tft_blk    = 47,  .tft_rst  = 21,  .tft_cs  = 14,  .tft_sck = 13,
    .tft_sda0   = 15,  .tft_sda1 = 16,  .tft_sda2 = 17, .tft_sda3 = 18,
    .touch_scl  = 12,  .touch_sda = 11, .touch_int = 9,  .touch_rst = 10,
    .enc_a      = 8,   .enc_b    = 7,
    .bat_adc    = 1,
    .btn        = 0,
    .mirror_x   = false, .mirror_y = false,
};

const board_pins_t board_k718 = {
    .name       = "JC3636K718",
    .tft_blk    = 21,  .tft_rst  = 17,  .tft_cs  = 12,  .tft_sck = 11,
    .tft_sda0   = 13,  .tft_sda1 = 14,  .tft_sda2 = 15, .tft_sda3 = 16,
    .touch_scl  = 10,  .touch_sda = 9,  .touch_int = 7,  .touch_rst = 8,
    .enc_a      = 2,   .enc_b    = 1,
    .bat_adc    = 6,
    .btn        = 0,
    .mirror_x   = true,  .mirror_y = true,
};

const board_pins_t *board = NULL;

void board_detect(void)
{
    board = &board_k518;
}

/* ---- In-memory NVS store ---- */

#define NVS_MAX_ENTRIES 32
#define NVS_KEY_LEN     16
#define NVS_BLOB_MAX    256

typedef struct {
    char     key[NVS_KEY_LEN];
    int64_t  int_value;
    uint8_t  blob[NVS_BLOB_MAX];
    size_t   blob_len;
    int      has_int;
    int      has_blob;
} nvs_entry_t;

static nvs_entry_t nvs_store[NVS_MAX_ENTRIES];
static int nvs_count = 0;

static nvs_entry_t *nvs_find(const char *key)
{
    int i;
    for (i = 0; i < nvs_count; i++) {
        if (strcmp(nvs_store[i].key, key) == 0)
            return &nvs_store[i];
    }
    return NULL;
}

static nvs_entry_t *nvs_find_or_create(const char *key)
{
    nvs_entry_t *e = nvs_find(key);
    if (e) return e;
    if (nvs_count >= NVS_MAX_ENTRIES) return NULL;
    e = &nvs_store[nvs_count++];
    memset(e, 0, sizeof(*e));
    snprintf(e->key, NVS_KEY_LEN, "%s", key);
    return e;
}

void sim_nvs_preset_i8(const char *key, int8_t value)
{
    nvs_entry_t *e = nvs_find_or_create(key);
    if (e) { e->int_value = value; e->has_int = 1; }
}

void sim_nvs_preset_i16(const char *key, int16_t value)
{
    nvs_entry_t *e = nvs_find_or_create(key);
    if (e) { e->int_value = value; e->has_int = 1; }
}

/* NVS API stubs */

esp_err_t nvs_flash_init(void) { return ESP_OK; }
esp_err_t nvs_flash_erase(void) { return ESP_OK; }

esp_err_t nvs_open(const char *namespace_name, int open_mode, nvs_handle_t *out_handle)
{
    (void)namespace_name;
    (void)open_mode;
    if (out_handle) *out_handle = 1;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) { (void)handle; }

esp_err_t nvs_get_i8(nvs_handle_t handle, const char *key, int8_t *out_value)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find(key);
    if (e && e->has_int) { *out_value = (int8_t)e->int_value; return ESP_OK; }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_get_i16(nvs_handle_t handle, const char *key, int16_t *out_value)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find(key);
    if (e && e->has_int) { *out_value = (int16_t)e->int_value; return ESP_OK; }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find(key);
    if (e && e->has_blob) {
        size_t copy = (e->blob_len < *length) ? e->blob_len : *length;
        memcpy(out_value, e->blob, copy);
        *length = copy;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find_or_create(key);
    if (e) { e->int_value = value; e->has_int = 1; return ESP_OK; }
    return ESP_FAIL;
}

esp_err_t nvs_set_i16(nvs_handle_t handle, const char *key, int16_t value)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find_or_create(key);
    if (e) { e->int_value = value; e->has_int = 1; return ESP_OK; }
    return ESP_FAIL;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    nvs_entry_t *e;
    (void)handle;
    e = nvs_find_or_create(key);
    if (e && length <= NVS_BLOB_MAX) {
        memcpy(e->blob, value, length);
        e->blob_len = length;
        e->has_blob = 1;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t nvs_commit(nvs_handle_t handle) { (void)handle; return ESP_OK; }

/* ---- LEDC (brightness PWM) ---- */

esp_err_t ledc_timer_config(const ledc_timer_config_t *c) { (void)c; return ESP_OK; }
esp_err_t ledc_channel_config(const ledc_channel_config_t *c) { (void)c; return ESP_OK; }
esp_err_t ledc_set_duty(ledc_mode_t m, ledc_channel_t ch, uint32_t d) { (void)m; (void)ch; (void)d; return ESP_OK; }
esp_err_t ledc_update_duty(ledc_mode_t m, ledc_channel_t ch) { (void)m; (void)ch; return ESP_OK; }

/* ---- Rotary encoder ---- */

knob_handle_t iot_knob_create(const knob_config_t *config)
{
    (void)config;
    return (knob_handle_t)1; /* non-NULL dummy */
}

esp_err_t iot_knob_delete(knob_handle_t h) { (void)h; return ESP_OK; }
esp_err_t iot_knob_register_cb(knob_handle_t h, knob_event_t e, knob_cb_t cb, void *d)
{
    (void)h; (void)e; (void)cb; (void)d;
    return ESP_OK;
}
esp_err_t iot_knob_unregister_cb(knob_handle_t h, knob_event_t e) { (void)h; (void)e; return ESP_OK; }
knob_event_t iot_knob_get_event(knob_handle_t h) { (void)h; return KNOB_NONE; }
int iot_knob_get_count_value(knob_handle_t h) { (void)h; return 0; }
esp_err_t iot_knob_clear_count_value(knob_handle_t h) { (void)h; return ESP_OK; }
esp_err_t iot_knob_resume(void) { return ESP_OK; }
esp_err_t iot_knob_stop(void) { return ESP_OK; }
esp_err_t knob_gpio_init(uint32_t gpio_num) { (void)gpio_num; return ESP_OK; }
esp_err_t knob_gpio_deinit(uint32_t gpio_num) { (void)gpio_num; return ESP_OK; }
uint8_t knob_gpio_get_key_level(void *gpio_num) { (void)gpio_num; return 0; }

/* ---- Battery ---- */

float sim_battery_voltage = 4.0f;

float knob_read_battery_voltage(void) { return sim_battery_voltage; }

/* ---- Random ---- */

uint32_t esp_random(void) { return (uint32_t)rand(); }
