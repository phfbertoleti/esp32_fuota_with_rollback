/* Module: NVS */

/* ESP32 / ESP-IDF specific header files */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_rw.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"

/* Define - debug */
#define NVS_TAG            "NVS"

/* Definição - namespace */
#define NAMESPACE_NVS      "esp32s3"

/* Static variables */
static SemaphoreHandle_t nvs_semaphore;

/* Function: init NVS module
 * Params: none
 * Return: none
 */
void init_nvs(void)
{
    esp_err_t ret;

    ESP_LOGI(NVS_TAG, "Initializing NVS module...\n");

    ret = nvs_flash_init();
    ESP_ERROR_CHECK( ret );

    nvs_semaphore = xSemaphoreCreateMutex();

    if( nvs_semaphore == NULL )
    {
        ESP_LOGE(NVS_TAG, "Failed to configure NVS semaphore");
    }                           
    else
    {
        ESP_LOGI(NVS_TAG, "NVS semaphore has been successfully configured");
    }

    ESP_LOGI(NVS_TAG, "NVS module initialized");
}

/* Function: store a string into NVS partition
 * Params: - pointer to array containing NVS key
 *         - pointer to array containing string to be stored
 * Return: ESP_OK: string sucessully stored into NVS partition
 *         anything else: failed to store string into NVS partition
 */
esp_err_t store_string_nvs(char * pt_key, char * pt_string)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_particao_nvs;

    if( xSemaphoreTake( nvs_semaphore, TIME_TO_WAIT_FOR_NVS_SEMAPHORE) != pdTRUE )
    { 
        ESP_LOGE(NVS_TAG, "Error: NVS semaphore isn't available");
        goto END_NVS_DATA_STORE;
    }

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: pointer to NVS key is null");
        ret = ESP_FAIL;
        goto END_NVS_DATA_STORE;
    }

    if (pt_string == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: pointer to string to be stored is null");
        ret = ESP_FAIL;
        goto END_NVS_DATA_STORE;
    }
    
    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_particao_nvs);    
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Failed to open NVS partition");
        goto END_NVS_DATA_STORE;  
    }

    ret = nvs_set_str(handler_particao_nvs, pt_key, pt_string);
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Failed to store string into NVS partition");
        goto END_NVS_DATA_STORE; 
    }

    ret = nvs_commit(handler_particao_nvs);
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Failed to commit NVS");
        goto END_NVS_DATA_STORE; 
    }

    ESP_LOGI(NVS_TAG, "String successfully stored into NVS partition");

END_NVS_DATA_STORE:
    xSemaphoreGive( nvs_semaphore );
    return ret;
}

/* Function: read a string from NVS partition
 * Params: - pointer to array containing NVS key
 *         - pointer to array to read stored string
 *         - string size
 * Return: ESP_OK: string sucessully read from NVS partition
 *         anything else: failed to read string from NVS partition
 */
esp_err_t read_string_nvs(char * pt_key, char * pt_string, size_t str_size)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_particao_nvs;

    if( xSemaphoreTake( nvs_semaphore, TIME_TO_WAIT_FOR_NVS_SEMAPHORE) != pdTRUE )
    { 
        ESP_LOGE(NVS_TAG, "Error: NVS semaphore isn't available");
        goto END_NVS_DATA_READ;
    }

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: pointer to NVS key is null");
        ret = ESP_FAIL;
        goto END_NVS_DATA_READ;
    }

    if (pt_string == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: pointer to array to put the read string is null");
        ret = ESP_FAIL;
        goto END_NVS_DATA_READ;
    }
    
    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_particao_nvs);    
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Failed to open NVS partition");
        goto END_NVS_DATA_READ;  
    }

    ret = nvs_get_str(handler_particao_nvs, pt_key, pt_string, &str_size);
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Failed to read string from NVS partition");
        goto END_NVS_DATA_READ;  
    }

    ESP_LOGI(NVS_TAG, "String has been successfully read from NVS partition");

END_NVS_DATA_READ:
    xSemaphoreGive( nvs_semaphore );
    return ret;
}