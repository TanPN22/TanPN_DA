#ifndef SEND_DATA_H
#define SEND_DATA_H

#include "cJSON.h"
#include "esp_log.h"
#include "app_main.h"

extern uint16_t timerCount;

void send_mqtt_set_state(esp_mqtt_client_handle_t client, int request_id, bool state);
void send_telemetry_data();
void sendData_Key(const char * const name,  const double number);
void send_alarm_json();
void send_sensor_data_via_mqtt(void);
void process_mqtt_data(esp_mqtt_client_handle_t client, const char *topic, int topic_len, const char *data, int data_len);
#endif