/* Module: ESP32 OTA */

/* ESP32 / ESP-IDF specific header files */
#include <string.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp32_ota.h"

/* Define - debug */
#define ESP32_OTA_TAG       "ESP32_OTA"

/* Static variables */
static const esp_partition_t *ota_partition = NULL;
static esp_ota_handle_t ota_handle = 0;
static bool can_OTA_start = false;
static int64_t t1 = 0;
static int64_t t2 = 0;

/* Function: init ESP32 OTA
 * Params: none
 * Return: none
 */
void init_esp32_ota(void)
{
    can_OTA_start = false;
    ESP_LOGI(ESP32_OTA_TAG, "ESP32 OTA module has been initialized");
}

/* Function: start OTA process
 * Params: none
 * Return: none
 */
void start_esp32_ota(void)
{
    esp_err_t err;

    if (can_OTA_start == true)
    {
        ESP_LOGE(ESP32_OTA_TAG, "OTA has already started. his call to start OTA will be ignored.");
        return;
    }

    ota_partition = esp_ota_get_next_update_partition(NULL);
    can_OTA_start = false;
    err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(ESP32_OTA_TAG, "Failed to start OTA. Error: %s", esp_err_to_name(err));
        return;
    }

    can_OTA_start = true;
    t1 = esp_timer_get_time() / 1000;
}

/* Function: OTA iterator
 * Params: - pointer to data array to be written into OTA_1 or OTA_2 partition
 *         - Amount of bytes to write
 * Return: none
 */
void iterator_esp32_ota(char *pt_rcv_bytes, int amount_rcv_bytes)
{
    esp_err_t err;
    uint32_t total = 0;

    if (can_OTA_start == false)
    {
        ESP_LOGE(ESP32_OTA_TAG, "OTA hasn't been started. OTA must be started before writting new firmware into OTA_1 or OTA_2 partitions");
        return;
    }

    /* Escreve pacote de OTA na partição disponível para OTA */
    err = esp_ota_write(ota_handle, pt_rcv_bytes, (size_t)amount_rcv_bytes);
    if (err != ESP_OK)
    {
        ESP_LOGE(ESP32_OTA_TAG, "Failed to write received bytes into OTA_1 or OTA_2 partitions. Error: %s", esp_err_to_name(err));
        can_OTA_start = false;
        return;
    }
    else
    {
        total = total + amount_rcv_bytes;
        ESP_LOGI(ESP32_OTA_TAG, "OTA: %u bytes have been download from TCP socket and written to OTA_1 or OTA_2 partitions", total);
    }
}

/* Function: end OTA
 * Params: none
 * Return: none
 */
void end_esp32_ota(void)
{
    esp_err_t err;

    /* Finaliza processo do OTA */
    err = esp_ota_end(ota_handle);
    if (err == ESP_OK)
    {
        err = esp_ota_set_boot_partition(ota_partition);
        if (err == ESP_OK)
        {
            t2 = esp_timer_get_time() / 1000;
            ESP_LOGI(ESP32_OTA_TAG, "OTA sucessfully done. OTA total time: %lld ms. ESP32 will restart in a second...", t2-t1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        else
        {
            ESP_LOGE(ESP32_OTA_TAG, "Failed to set boot partition. Error: %s", esp_err_to_name(err));
            return;
        }
    }
    else
    {
        ESP_LOGE(ESP32_OTA_TAG, "Failed to end OTA process. Error: %s", esp_err_to_name(err));
        return;
    }

    can_OTA_start = false;
}
