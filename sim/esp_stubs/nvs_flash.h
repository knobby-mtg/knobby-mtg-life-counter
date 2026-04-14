#ifndef _SIM_NVS_FLASH_H
#define _SIM_NVS_FLASH_H

#include "esp_err.h"

#define ESP_ERR_NVS_NO_FREE_PAGES  0x1100
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110

esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);

#endif /* _SIM_NVS_FLASH_H */
