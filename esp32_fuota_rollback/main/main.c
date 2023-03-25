/* ESP32 / ESP-IDF specific header files */
#include <stdio.h>
#include <esp_task_wdt.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

/* Header files from other project modules */
#include "nvs_rw/nvs_rw.h"
#include "wifi_st/wifi_st.h"
#include "esp32_ota/esp32_ota.h"
#include "breathing_light/breathing_light.h"
#include "fw_version.h"

/* Define - debug */
#define APP_MAIN_DEBUG_TAG      "APP_MAIN"

/* Define - maximum time without watchdog feed */
#define MAX_TIME_WITHOUT_WDT_FEED        60 //s

void app_main(void)
{
    esp_err_t err;
    
    esp_task_wdt_init(MAX_TIME_WITHOUT_WDT_FEED, true);
    
    ESP_LOGI(APP_MAIN_DEBUG_TAG, " ");
    ESP_LOGI(APP_MAIN_DEBUG_TAG, " ");
    ESP_LOGI(APP_MAIN_DEBUG_TAG, "----------------------------");
    ESP_LOGI(APP_MAIN_DEBUG_TAG, "Current firmware version: %s", FW_VERSION);
    ESP_LOGI(APP_MAIN_DEBUG_TAG, "----------------------------");
    ESP_LOGI(APP_MAIN_DEBUG_TAG, " ");
    ESP_LOGI(APP_MAIN_DEBUG_TAG, " ");

    /* Simulation of an error case: 
       reset ESP32 before marking OTA_1 or OTA_2 partitions as valid
    */
    //esp_restart();

    /* Init NVS module */
    init_nvs();

    /* Init breathing light module */
    init_breathing_light();

    /* Init ESP32 OTA module */
    init_esp32_ota();

    /* Init wifi module */
    wifi_init_sta();

    /* Mark current partition as valid */
    err = esp_ota_mark_app_valid_cancel_rollback();

    if (err != ESP_OK)
    {
        ESP_LOGE(APP_MAIN_DEBUG_TAG, "failed to mark current partition as valid. This is expected if the current partition is factory");
    }
    else
    {
        ESP_LOGI(APP_MAIN_DEBUG_TAG, "Current partition successfully marked as valid");
    }

    ESP_LOGI(APP_MAIN_DEBUG_TAG, "ESP32-S3 sucessfully initialized");
}
