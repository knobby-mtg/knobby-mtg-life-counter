#ifndef _SIM_NVS_H
#define _SIM_NVS_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

typedef uint32_t nvs_handle_t;

#define NVS_READONLY  0
#define NVS_READWRITE 1

esp_err_t nvs_open(const char *namespace_name, int open_mode, nvs_handle_t *out_handle);
void      nvs_close(nvs_handle_t handle);

esp_err_t nvs_get_i8(nvs_handle_t handle, const char *key, int8_t *out_value);
esp_err_t nvs_get_i16(nvs_handle_t handle, const char *key, int16_t *out_value);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length);

esp_err_t nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value);
esp_err_t nvs_set_i16(nvs_handle_t handle, const char *key, int16_t value);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length);

esp_err_t nvs_commit(nvs_handle_t handle);

#endif /* _SIM_NVS_H */
