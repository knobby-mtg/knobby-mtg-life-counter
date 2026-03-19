/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * 
 * Modified by planevina 2025-01-20
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*knob_cb_t)(void *, void *);
    typedef void *knob_handle_t;

    /**
     * @brief Knob events
     *
     */
    typedef enum
    {
        KNOB_LEFT = 0,  /*!< EVENT: Rotate to the left */
        KNOB_RIGHT,     /*!< EVENT: Rotate to the right */
        KNOB_EVENT_MAX, /*!< EVENT: Number of events */
        KNOB_NONE,      /*!< EVENT: No event */
    } knob_event_t;

    /**
     * @brief Knob config
     *
     */
    typedef struct
    {
        uint8_t gpio_encoder_a; /*!< Encoder Pin A */
        uint8_t gpio_encoder_b; /*!< Encoder Pin B */
    } knob_config_t;

    /**
     * @brief create a knob
     *
     * @param config pointer of knob configuration
     *
     * @return A handle to the created knob
     */
    knob_handle_t iot_knob_create(const knob_config_t *config);

    /**
     * @brief Delete a knob
     *
     * @param knob_handle A knob handle to delete
     *
     * @return
     *         - ESP_OK  Success
     *         - ESP_FAIL Failure
     */
    esp_err_t iot_knob_delete(knob_handle_t knob_handle);

    /**
     * @brief Register the knob event callback function
     *
     * @param knob_handle A knob handle to register
     * @param event Knob event
     * @param cb Callback function
     * @param usr_data user data
     *
     * @return
     *         - ESP_OK  Success
     *         - ESP_FAIL Failure
     */
    esp_err_t iot_knob_register_cb(knob_handle_t knob_handle, knob_event_t event, knob_cb_t cb, void *usr_data);

    /**
     * @brief Unregister the knob event callback function
     *
     * @param knob_handle A knob handle to register
     * @param event Knob event
     *
     * @return
     *         - ESP_OK  Success
     *         - ESP_FAIL Failure
     */
    esp_err_t iot_knob_unregister_cb(knob_handle_t knob_handle, knob_event_t event);

    /**
     * @brief Get knob event
     *
     * @param knob_handle A knob handle to register
     * @return knob_event_t Knob event
     */
    knob_event_t iot_knob_get_event(knob_handle_t knob_handle);

    /**
     * @brief Get knob count value
     *
     * @param knob_handle A knob handle to register
     *
     * @return int count_value
     */
    int iot_knob_get_count_value(knob_handle_t knob_handle);

    /**
     * @brief Clear knob cout value to zero
     *
     * @param knob_handle A knob handle to register
     *
     * @return
     *         - ESP_OK  Success
     *         - ESP_FAIL Failure
     */
    esp_err_t iot_knob_clear_count_value(knob_handle_t knob_handle);

    /**
     * @brief resume knob timer, if knob timer is stopped. Make sure iot_knob_create() is called before calling this API.
     *
     * @return
     *     - ESP_OK on success
     *     - ESP_ERR_INVALID_STATE   timer state is invalid.
     */
    esp_err_t iot_knob_resume(void);

    /**
     * @brief stop knob timer, if knob timer is running. Make sure iot_knob_create() is called before calling this API.
     *
     * @return
     *     - ESP_OK on success
     *     - ESP_ERR_INVALID_STATE   timer state is invalid
     */
    esp_err_t iot_knob_stop(void);

    /**
     * @brief Initialize a GPIO pin for knob input.
     *
     * This function configures a specified GPIO pin as an input for knob control.
     * It sets the pin mode, disables interrupts, and enables the pull-up resistor.
     *
     * @param gpio_num The GPIO number to be configured.
     * @return
     *      - ESP_OK: Configuration successful.
     *      - ESP_ERR_INVALID_ARG: Parameter error.
     *      - ESP_FAIL: Configuration failed.
     */
    esp_err_t knob_gpio_init(uint32_t gpio_num);

    /**
     * @brief Deinitialize a GPIO pin for knob input.
     *
     * This function resets the specified GPIO pin.
     *
     * @param gpio_num The GPIO number to be deinitialized.
     * @return
     *      - ESP_OK: Reset successful.
     *      - ESP_FAIL: Reset failed.
     */
    esp_err_t knob_gpio_deinit(uint32_t gpio_num);

    /**
     * @brief Get the level of a GPIO pin.
     *
     * This function returns the current level (high or low) of the specified GPIO pin.
     *
     * @param gpio_num The GPIO number to read the level from.
     * @return The level of the GPIO pin (0 or 1).
     */
    uint8_t knob_gpio_get_key_level(void *gpio_num);

#ifdef __cplusplus
}
#endif
