/* Módulo: socket tcp server (for OTA) */

/* ESP32 / ESP-IDF specific header files */
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
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include <esp_task_wdt.h>
#include "socket_tcp_server_wifi_OTA.h"

/* Header files from other project modules */
#include "../wifi_st/wifi_st.h"
#include "../esp32_ota/esp32_ota.h"

/* Header files - tasks priorities and stack sizes */
#include "../prio_tasks.h"
#include "../stacks_sizes.h"

/* Define - debug */
#define SOCKET_TCP_SERVER_TAG      "SOCKET_TCP_SERVER"

/* Defines - task parametrization */
#define PARAMS_TASK_SOCKET_TCP      NULL
#define CPU_SOCKET_TCP              1

/* Definition - time(s) since last firmware chuck received */
#define MAX_TIME_SINCE_LAST_FW_CHUNK         5000 // ms

/* Static variables */
static int listen_sock = 0;
static struct sockaddr_storage source_addr;
static char addr_str[128] = {0};
static int sock = 0;
static char socket_tcp_rx_buffer[WIFI_SOCKET_TCP_SERVER_RCV_BUFFER_SIZE] = {0};

/* Socket task handler */
TaskHandle_t socket_task_handler;

/* Task */
static void socket_tcp_server_task(void *arg);


/* Function: init TCP socket server (for receiving firmware chuncks)
 * Params: none
 * Return: none
 */
void socket_tcp_server_init(void)
{
    /* Inicializa a tarefa que gerencia o socket TCP */
    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "Initializing TCP socket server module...");
    xTaskCreatePinnedToCore(socket_tcp_server_task, "socket_tcp_server_task",
                            SOCKET_TCP_TASK_STACK_SIZE,
                            PARAMS_TASK_SOCKET_TCP,
                            PRIO_TASK_SOCKET_TCP,
                            &socket_task_handler,
                            CPU_SOCKET_TCP);

    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP socket server module initialized");
}

/* Function: TCP socket server task implementation
 * Params: task arguments
 * Return: none
 */
static void socket_tcp_server_task(void *arg)
{
    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    int received_bytes = 0;
    bool is_this_first_firmware_chunck = false;
    bool ota_initialized = false;
    int64_t time_of_last_firmware_chuck = 0;
    int64_t current_time = 0;
    int keep_alive = 1;
    int idle_keep_alive_time = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_IDLE;
    int interval_keep_alive_time = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_INTERVAL;
    int attempts_keep_alive = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_COUNT;
    socklen_t addr_len = sizeof(source_addr);
    bool is_there_any_client_connected = false;

    esp_task_wdt_add(NULL);
    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP socket server task initialized. Waiting wi-fi connection to be stablished...");

    while (get_status_wifi_connection() == false)
    {
        /* Check if wi-fi connection is available */
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "Wi-fi connection is stablished");

    /* Configure TCP socket server as IPv4 */
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(WIFI_PORT_SOCKET_TCP_SERVER);
    ip_protocol = IPPROTO_IP;

    /* Create TCP socket server. If it fails, TCP socket server is deleted */
    listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Failed to create TCP socket server (errno: %d)", errno);
        vTaskDelete(socket_task_handler);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP socket server successfully created");

    /* TCP socket server bind operation. If it fails, TCP socket server is deleted */
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Bind has failed (errno: %d)", errno);
        end_tcp_server_socket();
    }

    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "Bind successfully done. Port: %d", WIFI_PORT_SOCKET_TCP_SERVER);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Failed to listen (errno: %d)", errno);
        end_tcp_server_socket();
    }

    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP server socket in listen state (ready to get connections and firmware chunks)");

    /* Configure TCP server socket to work in non-blocking mode */
    int flags = fcntl(listen_sock, F_GETFL);
    fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK);
    is_there_any_client_connected = false;

    while (1)
    {
        esp_task_wdt_reset();

        /* Wait for client connection */
        if (is_there_any_client_connected == false)
        {
            sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

            if (sock >= 0)
            {
                /* Client is connected. Keep alive is configured */
                setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle_keep_alive_time, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval_keep_alive_time, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &attempts_keep_alive, sizeof(int));
                int flags_client = fcntl(sock, F_GETFL);
                fcntl(sock, F_SETFL, flags_client | O_NONBLOCK);
                
                if (source_addr.ss_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                }

                is_there_any_client_connected = true;
                ESP_LOGI(SOCKET_TCP_SERVER_TAG, "Client is connected. Client IP: %s", addr_str);                
            }
            else
            {
                is_there_any_client_connected = false;
            }

            if (is_there_any_client_connected == false)
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }
        }

        /* Check if there are bytes to be received from client.
           If yes, insert those bytes into received bytes buffer */
        memset(socket_tcp_rx_buffer, 0x00, sizeof(socket_tcp_rx_buffer));
        received_bytes = recv(sock, socket_tcp_rx_buffer, sizeof(socket_tcp_rx_buffer) - 1, MSG_DONTWAIT);
        
        if (received_bytes > 0)
        {
            if (is_this_first_firmware_chunck == false)
            {
                ESP_LOGI(SOCKET_TCP_SERVER_TAG, "OTA is starting...");
                start_esp32_ota();
                ota_initialized = true;
                is_this_first_firmware_chunck = true;
            }

            ESP_LOGI(SOCKET_TCP_SERVER_TAG, "%d bytes received from client", received_bytes);

            /* Send all received bytes for OTA processing */
            iterator_esp32_ota(socket_tcp_rx_buffer, received_bytes);
            time_of_last_firmware_chuck = esp_timer_get_time() / 1000;
        }
        else
        {
            /* If:
              - received_bytes = -1 (indicating no bytes have been received) AND
              - OTA is already started AND
              - These conditions are set for more time then defined in MAX_TIME_SINCE_LAST_FW_CHUNK
        
              It means OTA is ended (all firmware chunks have been received)
            */
            current_time = esp_timer_get_time() / 1000;

            if ((received_bytes == -1) && (ota_initialized == true) &&
                ((current_time - time_of_last_firmware_chuck) >= MAX_TIME_SINCE_LAST_FW_CHUNK))
            {
                ESP_LOGI(SOCKET_TCP_SERVER_TAG, "All firmware chunks have been received. Ending OTA...");
                end_esp32_ota();
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/* Function: end TCP server socket 
 * Params: none
 * Return: none
 */
void end_tcp_server_socket(void)
{
    close(listen_sock);
    vTaskDelete(socket_task_handler);
    ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP server socket is ended");
}