/* Header file: ESP32 OTA */

#ifndef HEADER_ESP32_OTA_MODULE
#define HEADER_ESP32_OTA_MODULE

#endif

/* Prototypes */
void init_esp32_ota(void);
void start_esp32_ota(void);
void iterator_esp32_ota(char *pt_rcv_bytes, int amount_rcv_bytes);
void end_esp32_ota(void);