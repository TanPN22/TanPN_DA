/*******************************************************************************
 *
 * Copyright (c) 2023
 * All Rights Reserved
 *
 *
 * Description: Smart Socket Project
 *
 * Author: TanPN
 *
 * Last Changed By:  $Author: TanPN $
 * Revision:         $Revision: 0.0 $
 * Last Changed:     $Date:  29/11/24 $
 *
 * Pls get info in readme.md
 ******************************************************************************/

#include "app_main.h"

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)
#define SMART_CONFIG 1
//====================Control============================
static const char *TAG = "Socket";
esp_mqtt_client_handle_t mqtt_client;

double IRMS;
double VRMS;

char ssid[33] = { 0 };
char password[65] = { 0 };

// Clock frequency and calibration coeficient
const float IRMS_COEF = 1.0/3680.30;
const float VRMS_COEF = 1.0/105.696;

bool socketStatus;
bool isWifihave = 0;

SensorData data2Send[DATA_SIZE];

bool ade7753Flag = 0;
bool flagSend = 0;
uint16_t dataCount = 0;

extern const uint8_t ca_cert_start[] asm("_binary_tb_cloud_root_ca_pem_start");
extern const uint8_t ca_cert_end[] asm("_binary_tb_cloud_root_ca_pem_end");
//=====================MQTT==============================

static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "password"}
    };

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = "/topic/test/response",
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

static void print_user_property(mqtt5_user_property_handle_t user_property)
{
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i ++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}


static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        esp_mqtt_client_reconnect(mqtt_client);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        esp_mqtt5_client_set_publish_property(client, &publish_property);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        print_user_property(event->property->user_property);
        process_mqtt_data(mqtt_client, event->topic, event->topic_len, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt5_app_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "/test/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = "mqtts://mqtt.thingsboard.cloud:8883",
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = "hqp7WA2TAYrBaawzch7d",
        .credentials.authentication.password = "",
        .session.last_will.topic = "/topic/will",
        .session.last_will.msg = "i will leave",
        .session.last_will.msg_len = 12,
        .session.last_will.qos = 1,
        .network.timeout_ms = -1,
        .broker.verification.certificate = (const char *)ca_cert_start,
        .broker.verification.certificate_len = ca_cert_end - ca_cert_start,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
    mqtt_client = client;

    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(client, &connect_property);

    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void ade7753_read_task(void *arg) {
#if SPI_TURN_ON
    #if ADE_WAVEFORM
    ADE7753_begin();
    
    ADE7753_setInterrupt(ADE7753_WSMP);

    ADE7753_setWaveSelect(ADE7753_WAV_CH1);//Set channel 2
    ADE7753_setDataRate(ADE7753_DR_1_1024); 
    #else
    ADE7753_begin();

    #endif

    ADE7753_setCh1Gain(ADE7753_GAIN_4);
    ADE7753_setCh2Gain(ADE7753_GAIN_2);  

    ADE7753_setCh1FullScale(ADE7753_FS_0_5V);

    const TickType_t delay_1s = pdMS_TO_TICKS(1000);  
    TickType_t last_mqtt_time = xTaskGetTickCount();

    while (1) {
        //Do toc do doc nhanh nen dung ngat khong hop li
    // if (ade7753Flag == 1)
    // {
    //     ade7753Flag = 0;

        #if ADE_WAVEFORM
        if (ADE7753_status() & ADE7753_WSMP) {
            // Show waveform value in serial plotter
            int32_t value = ADE7753_readWaveForm();
            ESP_LOGI(TAG,"%ld", value);
        }
        #else
        IRMS = (float)IRMS_COEF * ADE7753_readCurrentRMS();
        VRMS = VRMS_COEF * ADE7753_readVoltageRMS();

        //Doan nay la gop data thanh 1 mang roi moi gui
        //Ham gui data MQTT
        // data2Send[dataCount].voltage = VRMS;
        // data2Send[dataCount].current = IRMS;
        // dataCount ++;

        // if (dataCount == DATA_SIZE){
        //     dataCount = 0;
        //     send_sensor_data_via_mqtt();
        // }

        // ESP_LOGI(TAG,"IRMS: %.2f A,  VRMS: %.2f V", IRMS, VRMS);

        #endif

        if ((xTaskGetTickCount() - last_mqtt_time) >= delay_1s) {
            send_telemetry_data();  // Gọi hàm gửi dữ liệu
            last_mqtt_time = xTaskGetTickCount();  // Cập nhật thời gian gửi
        }
    // }
        //ESP_ROM... doc tan so 10kHz.
        // esp_rom_delay_us(100);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
#endif // USE_SPI
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef SMART_CONFIG
    bool ssid_exists = read_from_nvs("ssid", ssid, sizeof(ssid));
    bool uuid_exists = read_from_nvs("uuid", password, sizeof(password));

    load_socket_status();

    gpio_Init();

    if (ssid_exists && uuid_exists){
        ESP_LOGI(TAG, "WiFi is already");
        isWifihave = 1;
        
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = "",
                .password = "",
            },
        };

        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        esp_wifi_start();
    }else {
        ESP_LOGI(TAG, "No WiFI, SmartConfig");
        isWifihave = 0;
        initialise_wifi(); 
    }

    while (isWifihave == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
#else
    ESP_ERROR_CHECK(example_connect());
#endif
    
    #if SPI_TURN_ON
    xTaskCreatePinnedToCore(
        ade7753_read_task,     
        "ADE7753_Read_Task",     
        ADE7753_STACK_SIZE,      
        NULL,                
        ADE7753_TASK_PRIORITY,   
        NULL,                    
        ADE7753_TASK_CORE      
    );
    #endif

    mqtt5_app_start();
}
