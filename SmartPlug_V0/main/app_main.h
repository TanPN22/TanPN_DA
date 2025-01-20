#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdio.h>
#include <stdint.h>
#include "stdlib.h"
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "time.h"
#include "esp_wifi.h"
#include "esp_netif_ip_addr.h"
#include "math.h"
#include "driver/gpio.h"

#include "app_main.h"
#include "send_data.h"
#include "ADE7753.h"

#define PIN_RELAY           23
#define PIN_LED_R           33
#define PIN_LED_G           25
#define PIN_LED_B           32
#define PIN_LEAK            14
#define PIN_IQR             18

#define ADE7753_TASK_CORE 1
#define ADE7753_TASK_PRIORITY 5  
#define ADE7753_STACK_SIZE 4096 
#define ESP_INTR_FLAG_DEFAULT 0

//IF define
#define SPI_TURN_ON 1
#define FAKE_DATA 0
#define ADE_WAVEFORM 0
#define DATA_SIZE 1000

extern bool socketStatus;
extern bool isWifihave;

extern uint8_t modeADE7753;

extern char ssid[33];
extern char password[65];

extern float data2Send[DATA_SIZE];
extern uint16_t dataCount;
extern bool flagSend;

extern esp_mqtt_client_handle_t mqtt_client;

extern bool ade7753Flag;

extern double IRMS;
extern double VRMS;

void gpio_Init(void);
void save_socket_status(bool status);
bool load_socket_status();
void initialise_wifi(void);
void socketControl(bool _status);
bool read_from_nvs(const char *key, char *value, size_t length);
void save_to_nvs(const char *key, const char *value);
void clear_wifi_from_nvs();
void wifi_init_sta(const char *ssid, const char *password);
#endif