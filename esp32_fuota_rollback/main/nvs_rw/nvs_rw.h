/* Header file: NVS module */

#ifndef HEADER_NVS_MODULE
#define HEADER_NVS_MODULE

/* Define -semaphore */
#define TIME_TO_WAIT_FOR_NVS_SEMAPHORE    ( TickType_t ) 1

#endif

/* Protótipos */
void init_nvs(void);
esp_err_t store_string_nvs(char * pt_key, char * pt_string);
esp_err_t read_string_nvs(char * pt_key, char * pt_string, size_t tam_str);