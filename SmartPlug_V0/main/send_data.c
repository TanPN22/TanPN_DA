#include "send_data.h"
static const char *TAG = "Socket";
#include "freertos/timers.h"
static TimerHandle_t timerHandle;
uint16_t timerCount = 0;
// Response function MQTT type {"method":"setState","params":true/false}
void send_mqtt_set_state(esp_mqtt_client_handle_t client, int request_id, bool state) {
    // Create topic response
    char response_topic[50];
    snprintf(response_topic, sizeof(response_topic), "v1/devices/me/rpc/response/%d", request_id);

    // Create json
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "method", "getState");
    cJSON_AddBoolToObject(response_json, "params", state);
    char *response_data = cJSON_PrintUnformatted(response_json);

    // Send data to ThingsBoard
    int msg_id = esp_mqtt_client_publish(client, response_topic, response_data, 0, 1, 0);
    ESP_LOGI(TAG, "Sent response to %s, msg_id=%d, data=%s", response_topic, msg_id, response_data);

    // Free memory
    cJSON_Delete(response_json);
    free(response_data);
}

// Create and add data to json field
void send_telemetry_data() {
    // Create JSON
    cJSON *root = cJSON_CreateObject();

    // Create random data
    bool socket_status = rand() % 2;                                  

    // Add the field to json
    cJSON_AddBoolToObject(root, "Socket_status", socket_status);
    cJSON_AddNumberToObject(root, "Voltage", VRMS);
    cJSON_AddNumberToObject(root, "Current", IRMS);
    cJSON_AddNumberToObject(root, "Power", VRMS*IRMS);

    // Convert json data to string
    char *json_string = cJSON_Print(root);

    // Send data to ThingsBoard passing over MQTT
    esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", json_string, 0, 1, 0);

    // free the memory
    cJSON_Delete(root);
    free(json_string);

    // ESP_LOGI("Telemetry Task", "Data sent: voltage=%.2f, current=%.2f, power=%.2f, ",
            //  voltage, current, power);
}

void send_sensor_data_via_mqtt(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON *data_array = cJSON_CreateArray();

    // Duyệt qua mảng data2Send và thêm từng phần tử vào JSON array
    for (int i = 0; i < DATA_SIZE; i++) {
        cJSON *data_object = cJSON_CreateObject();
        cJSON_AddNumberToObject(data_object, "voltage", data2Send[i].voltage);
        cJSON_AddNumberToObject(data_object, "current", data2Send[i].current);
        cJSON_AddItemToArray(data_array, data_object);
    }

    cJSON_AddItemToObject(root, "data", data_array);

    char *json_string = cJSON_PrintUnformatted(root);

    // Gửi chuỗi JSON qua MQTT
    int msg_id = esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", json_string, 0, 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI("MQTT", "Data sent successfully, msg_id=%d", msg_id);
    } else {
        ESP_LOGE("MQTT", "Failed to send data.");
    }

    // Giải phóng bộ nhớ JSON
    cJSON_Delete(data_array);
    free(json_string);
}

void send_timer_json(int timerCount) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "timer", timerCount);

    char *json_string = cJSON_PrintUnformatted(root);

    int msg_id = esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", json_string, 0, 1, 0);

    cJSON_Delete(root);
    free(json_string);
}

void send_alarm_json() {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "alarm", 1);

    char *json_string = cJSON_PrintUnformatted(root);

    int msg_id = esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", json_string, 0, 1, 0);

    cJSON_Delete(root);
    free(json_string);
}

void timer_callback(TimerHandle_t xTimer) {
    if (timerCount > 0) {
        timerCount--;
        ESP_LOGI(TAG, "timerCount: %d", timerCount);

        send_timer_json(timerCount);

        if (timerCount == 0) {
            ESP_LOGI(TAG, "Timer expired.");
            bool statusBuffer = !socketStatus;
            socketControl(statusBuffer);
            xTimerStop(xTimer, 0);
        }
    }
}



// Process data from MQTT
void process_mqtt_data(esp_mqtt_client_handle_t client, const char *topic, int topic_len, const char *data, int data_len) {
    ESP_LOGI(TAG, "Received Data:");
    ESP_LOGI(TAG, "TOPIC=%.*s", topic_len, topic);
    ESP_LOGI(TAG, "DATA=%.*s", data_len, data);

    // copy json str
    char *json_str = strndup(data, data_len);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON data");
        return;
    }

    //  Parse json string
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        ESP_LOGE(TAG, "Invalid JSON format");
        free(json_str);
        return;
    }
    // check "method" field JSON
    cJSON *method = cJSON_GetObjectItem(json, "method");
    if (cJSON_IsString(method) && strcmp(method->valuestring, "getState") == 0) {
        ESP_LOGI(TAG, "Received getState method. Processing...");

        // Boolen status
        bool current_state = false;

        // Get id from topic
        int request_id;
        sscanf(topic + strlen("v1/devices/me/rpc/request/"), "%d", &request_id);

        // Respon to thingsboard
        send_mqtt_set_state(client, request_id, current_state);
    }

    if (strcmp(method->valuestring, "setState") == 0) {
        //Get param info
        cJSON *params = cJSON_GetObjectItem(json, "params");
        if (cJSON_IsBool(params)) {
            // Turn on led
            socketStatus = cJSON_IsTrue(params);
            save_socket_status(socketStatus);
            gpio_set_level(PIN_RELAY, socketStatus ? 1 : 0);
            gpio_set_level(PIN_LED_G, socketStatus ? 1 : 0);
            ESP_LOGI(TAG, "LED %s", socketStatus ? "ON" : "OFF");
        } else {
            ESP_LOGE(TAG, "Invalid 'params' field");
        }
    }

    if (strcmp(method->valuestring, "UPTIME") == 0) {
        if (timerCount == 0) {
            // Nếu timerCount = 0, tạo Timer nếu chưa có
            if (timerHandle == NULL) {
                timerHandle = xTimerCreate(
                    "Timer",                 // Tên Timer
                    pdMS_TO_TICKS(1000),      // Chu kỳ 1 giây
                    pdTRUE,                   // Tự động lặp lại
                    (void *)0,                // Không cần dữ liệu callback
                    timer_callback            // Hàm callback
                );

                if (timerHandle == NULL) {
                    ESP_LOGE(TAG, "Failed to create timer");
                    return;
                }
            }

            timerCount = 30;  // Gán giá trị đếm ngược ban đầu là 30 giây
            ESP_LOGI(TAG, "Timer started with 30 seconds.");
            xTimerStart(timerHandle, 0);  // Khởi động Timer
        } else {
            timerCount += 30;  // Nếu Timer đang chạy, cộng thêm 30 giây
            ESP_LOGI(TAG, "Added 30 seconds, new timerCount: %d", timerCount);
        }

    }

    if (strcmp(method->valuestring, "DOWNTIME") == 0) {
        if (timerCount <= 30)
        {
            send_timer_json(0);
            timerCount = 0;
            bool statusBuffer = !socketStatus;
            socketControl(statusBuffer);
        }else {
            timerCount -= 30;
        }
    }

    // free memory
    cJSON_Delete(json);
    free(json_str);
}