/* Header file: socket tcp server */

#ifndef HEADER_SOCKET_TCP_OTA_SERVER_MODULE
#define HEADER_SOCKET_TCP_OTA_SERVER_MODULE

/* Defines - TCP socket server */
#define WIFI_PORT_SOCKET_TCP_SERVER                  CONFIG_ESP_WIFI_PORT_SOCKET_TCP_SERVER
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_IDLE        CONFIG_ESP_WIFI_SOCKET_TCP_SERVER_KEEPALIVE_IDLE
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_INTERVAL    CONFIG_ESP_WIFI_SOCKET_TCP_SERVER_KEEPALIVE_INTERVAL
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_COUNT       CONFIG_ESP_WIFI_SOCKET_TCP_SERVER_KEEPALIVE_COUNT
#define WIFI_SOCKET_TCP_SERVER_RCV_BUFFER_SIZE       CONFIG_ESP_WIFI_SOCKET_TCP_SERVER_RCV_BUFFER_SIZE

#endif

/* Protótipos */
void socket_tcp_server_init(void);
void end_tcp_server_socket(void);