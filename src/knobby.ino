#include <WiFi.h>
#include "esp_bt.h"

#include "scr_st77916.h"
#include <lvgl.h>
#include "hal/lv_hal.h"
#include "SD_MMC.h"
#include "FS.h"
#include <sdmmc_cmd.h>
#include "knob.h"

static const int BATTERY_ADC_PIN = 1;
static const float BATTERY_DIVIDER_RATIO = 2.0f;
static const float BATTERY_CALIBRATION_SCALE = 1.0f;
static const float BATTERY_CALIBRATION_OFFSET = 0.0f;
static float battery_voltage_filtered = 0.0f;
static bool battery_voltage_has_value = false;
static bool firmware_ready = false;

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
  // 160 MHz verbraucht weniger Batterie, 240 MHz reduziert hier aber Darstellungsfehler und Hänger.
  setCpuFrequencyMhz(240);

  // Funk deaktivieren
  WiFi.mode(WIFI_OFF);
  btStop();

  delay(200);
  Serial.begin(115200);

  firmware_ready = scr_lvgl_init();
  if (!firmware_ready) {
    Serial.println("Firmware initialization failed; UI startup aborted.");
    return;
  }

  knob_gui();
}

void loop()
{
  if (!firmware_ready) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    return;
  }

  knob_process_pending();
  lv_timer_handler();
  vTaskDelay(5);
}
