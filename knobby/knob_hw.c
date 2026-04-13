#include "knob_hw.h"
#include "knob_nvs.h"
#include "driver/ledc.h"

// ---------- private constants ----------
#include "pincfg.h"
#define BACKLIGHT_PIN TFT_BLK
#define BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_FREQ 5000
#define BACKLIGHT_LEDC_RES LEDC_TIMER_10_BIT
#define BACKLIGHT_DUTY_MAX 1023

// Tunable power/timing constants are defined in knob_hw.h

// ---------- state ----------
int brightness_percent = DEFAULT_BRIGHTNESS_PERCENT;
int auto_dim_setting = AUTO_DIM_OFF;
bool dimmed = false;
float battery_voltage = 0.0f;
int battery_percent = -1;

static uint32_t battery_sample_tick = 0;
static bool battery_sample_valid = false;
static uint32_t last_activity_tick = 0;
static uint32_t undim_tick = 0;
static lv_timer_t *auto_dim_timer = NULL;

// ---------- battery curve ----------
static const float battery_curve_voltages[] = {
    3.35f, 3.55f, 3.68f, 3.74f, 3.80f, 3.88f, 3.96f, 4.06f, 4.18f
};
static const int battery_curve_percentages[] = {
    0, 5, 12, 22, 34, 48, 64, 82, 100
};

// ---------- private helpers ----------
static int clamp_percent(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

static int battery_percent_from_voltage(float voltage)
{
    size_t i;

    if (voltage <= battery_curve_voltages[0]) return 0;
    for (i = 1; i < (sizeof(battery_curve_voltages) / sizeof(battery_curve_voltages[0])); i++) {
        if (voltage <= battery_curve_voltages[i]) {
            float low_v = battery_curve_voltages[i - 1];
            float high_v = battery_curve_voltages[i];
            int low_p = battery_curve_percentages[i - 1];
            int high_p = battery_curve_percentages[i];
            float ratio = (voltage - low_v) / (high_v - low_v);
            return clamp_percent((int)(low_p + ((high_p - low_p) * ratio) + 0.5f));
        }
    }

    return 100;
}

// ---------- battery ----------
void update_battery_measurement(bool force)
{
    if (!force && battery_sample_valid && (lv_tick_elaps(battery_sample_tick) < BATTERY_SAMPLE_INTERVAL_MS)) {
        return;
    }

    battery_voltage = knob_read_battery_voltage();
    battery_sample_tick = lv_tick_get();
    battery_sample_valid = (battery_voltage > 0.0f);
}

int read_battery_percent(void)
{
    update_battery_measurement(false);
    if (!battery_sample_valid) return -1;
    return battery_percent_from_voltage(battery_voltage);
}

// ---------- brightness ----------
static void brightness_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .duty_resolution = BACKLIGHT_LEDC_RES,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = BACKLIGHT_LEDC_FREQ,
        .clk_cfg = LEDC_USE_RTC8M_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = BACKLIGHT_PIN,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

void brightness_apply(void)
{
    uint32_t duty = (uint32_t)((brightness_percent * BACKLIGHT_DUTY_MAX) / 100);
    ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty);
    ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL);
}

void change_brightness(int delta)
{
    brightness_percent = clamp_brightness(brightness_percent + delta);
    nvs_set_brightness(brightness_percent);
    brightness_apply();
}

// ---------- auto-dim ----------
bool activity_kick(void)
{
    bool was_dimmed = dimmed;
    last_activity_tick = lv_tick_get();
    if (dimmed) {
        if (auto_dim_timer != NULL) {
            lv_timer_resume(auto_dim_timer);
        }
        dimmed = false;
        undim_tick = last_activity_tick;
        brightness_apply();
    }
    return was_dimmed;
}

bool in_undim_grace(void)
{
    return undim_tick != 0 && lv_tick_elaps(undim_tick) < UNDIM_GRACE_MS;
}

static void auto_dim_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (auto_dim_setting == AUTO_DIM_OFF || dimmed) return;
    uint32_t timeout = auto_dim_ms[auto_dim_setting];
    if (lv_tick_elaps(last_activity_tick) >= timeout) {
        dimmed = true;
        uint32_t duty = (uint32_t)((AUTO_DIM_BRIGHTNESS * BACKLIGHT_DUTY_MAX) / 100);
        ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty);
        ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL);
        if (auto_dim_timer != NULL) {
            lv_timer_pause(auto_dim_timer);
        }
    }
}

// ---------- init ----------
void knob_hw_init(void)
{
    knob_nvs_init();
    brightness_init();
    brightness_percent = nvs_get_brightness();
    auto_dim_setting = nvs_get_auto_dim();
    last_activity_tick = lv_tick_get();
    auto_dim_timer = lv_timer_create(auto_dim_timer_cb, AUTO_DIM_CHECK_PERIOD_MS, NULL);
}
