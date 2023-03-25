/* Module: wi-fi (station) */

/* ESP32 / ESP-IDF specific header files */
#include <string.h>
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
#include "wifi_st.h"

/* Header files from other project modules */
#include "../nvs_rw/nvs_rw.h"
#include "../socket_tcp_server_wifi_OTA/socket_tcp_server_wifi_OTA.h"

/* Define - debug */
#define WIFI_TAG                "WIFI"

/* Static variables */
static bool is_wifi_connected = false;

/* Local functions */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static bool init_wifi_st(uint8_t * pt_ssid, uint8_t * pt_pass);

/* Function: inform the wi-fi connection status
 * Params: none
 * Return: true: wi-fi is connected
 *         false: wi-fi isn't connected
 */
bool get_status_wifi_connection(void)
{
    return is_wifi_connected;
}

/* Function: wi-fi event handler
 * Params: events and arguments
 * Return: none
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        ESP_LOGI(WIFI_TAG, "Connecting to wi-fi network...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        is_wifi_connected = false;
        ESP_LOGI(WIFI_TAG, "Wi-fi is now disconnected. Reconnecting to wi-fi network...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        is_wifi_connected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "Wi-fi connection is stablished. IP:" IPSTR, IP2STR(&event->ip_info.ip));
        socket_tcp_server_init();
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(WIFI_TAG, "station "MACSTR" is disconnected, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/* Function: wi-fi init
 * Params: none
 * Return: none
 */
void wifi_init_sta(void)
{
    uint8_t wifi_cred_ssid[TAM_MAX_SSID_ST_WIFI] = {0};
    uint8_t wifi_cred_pass[TAM_MAX_PASS_ST_WIFI] = {0};
    size_t str_size = 0;
    bool wifi_st_init_status = true;
    
    /* Read wi-fi credentials.
       If they cannot be read, default credentials will be set */
    str_size = sizeof(wifi_cred_ssid);   
    if (read_string_nvs(CHAVE_SSID_WIFI, (char *)wifi_cred_ssid, str_size) == ESP_OK)
    {
        ESP_LOGI(WIFI_TAG, "SSID successfully read: %s\n", wifi_cred_ssid);
    }
    else
    {
        snprintf((char *)wifi_cred_ssid, sizeof(wifi_cred_ssid), "%s", WIFI_SSID_ST_DEFAULT);
        ESP_LOGI(WIFI_TAG, "Failed to read SSID. SSID configured: %s", wifi_cred_ssid);
    }

    str_size = sizeof(wifi_cred_pass);   
    if (read_string_nvs(CHAVE_PASS_WIFI, (char *)wifi_cred_pass, str_size) == ESP_OK)
    {
        ESP_LOGI(WIFI_TAG, "Password sucessfully read");
    }
    else
    {
        snprintf((char *)wifi_cred_pass, sizeof(wifi_cred_pass), "%s", WIFI_PASS_ST_DEFAULT);
        ESP_LOGI(WIFI_TAG, "Failed to read password");
    }

    /* Init wifi */
    is_wifi_connected = false;
    ESP_LOGI(WIFI_TAG, "Initializing wi-fi (station)...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    /* Set static IP for ESP32 */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    esp_netif_dhcpc_stop(sta_netif);
    esp_netif_ip_info_t ip_fixo;
    ip_fixo.ip.addr = ipaddr_addr(WIFI_ST_STATIC_IP);
    ip_fixo.gw.addr = ipaddr_addr(WIFI_ST_GATEWAY);
    ip_fixo.netmask.addr = ipaddr_addr(WIFI_ST_NETWORK_MASK);
    esp_netif_set_ip_info(sta_netif, &ip_fixo);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Registra callbacks para eventos de wifi e eventos relacionados a conexao (ip na rede) */
    ESP_LOGI(WIFI_TAG, "Registering wi-fi callbacks...");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_LOGI(WIFI_TAG, "Callbacks de eventos wifi e eventos de conexao de rede registrados\n");
 
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    /* Configura SSID, senha e segurança do wi-fi */ 
    /* Station */
    wifi_st_init_status = init_wifi_st(wifi_cred_ssid, wifi_cred_pass);

    if (wifi_st_init_status == true)
    {
        ESP_LOGI(WIFI_TAG, "Wi-fi sucessfully initialized (station)");   
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "Failed to initialize Wi-fi (station)");   
    }
}

/* Function: init station mode
 * Params: - pointer to array containing ssid
 *         - pointer to array containing password
 * Return: true: wi-fi station mode initialized
 *         false: failed to initialize wi-fi station mode
 */
static bool init_wifi_st(uint8_t * pt_ssid, uint8_t * pt_pass)
{
    bool status_wifi_st = false;

    if (pt_ssid == NULL)
    {
        ESP_LOGE(WIFI_TAG, "Pointer to SSID is null");
        goto END_WIFI_STA_INIT;
    }

    if (pt_pass == NULL)
    {
        ESP_LOGE(WIFI_TAG, "Pointer to password is null");
        goto END_WIFI_STA_INIT;
    }

    wifi_config_t wifi_config = {0};
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", (char *)pt_ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", (char *)pt_pass);
    
    esp_err_t ret_esp_set_mode = esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t ret_esp_wifi_set_config = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_connect();
    
    if ( (ret_esp_set_mode == ESP_OK) && (ret_esp_wifi_set_config == ESP_OK) )
    {
        status_wifi_st = true;
    }
    else
    {
        status_wifi_st = false;
    }
      
    if (status_wifi_st == true)
    {
        ESP_LOGI(WIFI_TAG, "Wi-fi station successfully initialized");
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "Failed to initialize wi-fi station");
    }

END_WIFI_STA_INIT:
    return status_wifi_st;
}