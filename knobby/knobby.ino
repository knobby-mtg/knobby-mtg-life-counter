#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "soc/usb_serial_jtag_struct.h"

#include "scr_st77916.h"
#include <lvgl.h>
#include "hal/lv_hal.h"
#include "knob.h"
#include "knob_hw.h"

static const int BATTERY_ADC_PIN = 1;
static const float BATTERY_DIVIDER_RATIO = 2.0f;
static const float BATTERY_CALIBRATION_SCALE = 1.0f;
static const float BATTERY_CALIBRATION_OFFSET = 0.0f;
static float battery_voltage_filtered = 0.0f;
static bool battery_voltage_has_value = false;

extern "C" float knob_read_battery_voltage(void)
{
  uint32_t millivolts = 0;
  uint32_t sum = 0;
  uint32_t min_sample = UINT32_MAX;
  uint32_t max_sample = 0;
  const int sample_count = 16;
  float measured_voltage = 0.0f;

  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  for (int i = 0; i < sample_count; i++) {
    uint32_t sample = analogReadMilliVolts(BATTERY_ADC_PIN);
    sum += sample;
    if (sample < min_sample) min_sample = sample;
    if (sample > max_sample) max_sample = sample;
    delayMicroseconds(250);
  }

  sum -= min_sample;
  sum -= max_sample;
  millivolts = sum / (sample_count - 2);
  if (millivolts == 0) {
    return 0.0f;
  }

  measured_voltage = (((float)millivolts * BATTERY_DIVIDER_RATIO) / 1000.0f);
  measured_voltage = (measured_voltage * BATTERY_CALIBRATION_SCALE) + BATTERY_CALIBRATION_OFFSET;

  if (!battery_voltage_has_value) {
    battery_voltage_filtered = measured_voltage;
    battery_voltage_has_value = true;
  } else {
    battery_voltage_filtered = (battery_voltage_filtered * 0.7f) + (measured_voltage * 0.3f);
  }

  return battery_voltage_filtered;
}

void setup()
{
  // Use the configured active CPU frequency for easier tuning.
  setCpuFrequencyMhz(CPU_FREQ_ACTIVE);

  // Disable radios
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  delay(200);
  Serial.begin(115200);

  scr_lvgl_init();
  knob_gui();

  // Keep RTC8M clock alive during light sleep so LEDC PWM (backlight) continues
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_ON);
  gpio_sleep_sel_dis((gpio_num_t)TFT_BLK);

  // Configure light sleep wakeup sources
  gpio_wakeup_enable((gpio_num_t)TOUCH_PIN_NUM_INT, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  // Timer wakeup duration is set dynamically in loop() from lv_timer_handler()'s
  // next-deadline value so the CPU only wakes when LVGL actually needs to run.
}

// Minimum idle interval before using light sleep.
// Below this threshold we fall back to vTaskDelay to avoid sleep/wake overhead.
#define ACTIVE_SLEEP_MIN_MS 10U

// Detect active USB host by checking if the SOF frame counter is advancing.
// USB hosts send Start-of-Frame every 1ms; a changing counter means plugged in.
static bool usb_host_active(void)
{
  static uint32_t prev_sof = 0;
  uint32_t sof = USB_SERIAL_JTAG.fram_num.sof_frame_index;
  bool active = (sof != prev_sof);
  prev_sof = sof;
  return active;
}

void loop()
{
  uint32_t time_till_next;

  knob_process_pending();
  time_till_next = lv_timer_handler();

  if (time_till_next >= ACTIVE_SLEEP_MIN_MS && !usb_host_active()) {
    uint8_t level_a = gpio_get_level((gpio_num_t)ROTARY_ENC_PIN_A);
    uint8_t level_b = gpio_get_level((gpio_num_t)ROTARY_ENC_PIN_B);
    gpio_wakeup_enable((gpio_num_t)ROTARY_ENC_PIN_A, level_a ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable((gpio_num_t)ROTARY_ENC_PIN_B, level_b ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_timer_wakeup((uint64_t)time_till_next * 1000ULL);
    esp_light_sleep_start();
    gpio_wakeup_disable((gpio_num_t)ROTARY_ENC_PIN_A);
    gpio_wakeup_disable((gpio_num_t)ROTARY_ENC_PIN_B);
  } else {
    gpio_wakeup_disable((gpio_num_t)ROTARY_ENC_PIN_A);
    gpio_wakeup_disable((gpio_num_t)ROTARY_ENC_PIN_B);
    vTaskDelay(pdMS_TO_TICKS(time_till_next));
  }
}
