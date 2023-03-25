/* Module: breathing light */

/* ESP32 / ESP-IDF specific header files */
#include <string.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

/* Header files from other project modules */
#include "../prio_tasks.h"
#include "../stacks_sizes.h"

/* Define - debug */
#define BREATHING_LIGHT_TAG                "BREATHING_LIGHT"

/* Define - LED on/off times */
#define LED_ON_OFF_TIMES                  500 //ms

/* Defines - LED specific GPIO and PINSEL */
#define GPIO_LED                17
#define GPIO_LED_OUTPUT_PIN_SEL (1ULL << GPIO_LED)

/* Defines - task parametrization */
#define PARAMS_TASK_BREATHING_LIGHT    NULL
#define CPU_BREATHING_LIGHT            1

/* Breathing light task handler */
TaskHandle_t handler_breathing_light;

/* Tasks */
static void breathing_light_task(void *arg);

/* Function: init breathing light module
 * Params: none
 * Return: none
 */
void init_breathing_light(void)
{
    gpio_config_t io_conf = {};

    /* Init breathing light GPIO */
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_LED_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(breathing_light_task, "breathing_light_task",
                            BREATHING_LIGHT_TASK_STACK_SIZE,
                            PARAMS_TASK_BREATHING_LIGHT,
                            PRIO_TASK_BREATHING_LIGHT,
                            &handler_breathing_light,
                            CPU_BREATHING_LIGHT);

    ESP_LOGI(BREATHING_LIGHT_TAG, "Breathing light module has been initalized");
}

/* Function: breathing light task implementation
 * Params: task arguments
 * Return: none
 */
static void breathing_light_task(void *arg)
{ 
    esp_task_wdt_add(NULL);
    ESP_LOGI(BREATHING_LIGHT_TAG, "Breathing light initialized");

    /* Blink breathing light */
    while (1)
    {
        esp_task_wdt_reset();
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(LED_ON_OFF_TIMES / portTICK_PERIOD_MS);

        esp_task_wdt_reset();        
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(LED_ON_OFF_TIMES / portTICK_PERIOD_MS);
    }
}