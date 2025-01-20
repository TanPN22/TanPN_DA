#include "app_main.h"
#include "esp_log.h"
#include "math.h"
static const char *TAG = "Socket";

bool checkZeroCross(void);

//ISR for ADE7753
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    int gpio_num = (int) arg;

    switch (gpio_num)
    {
    case PIN_IQR:
        ade7753Flag = 1;
        break;

    case PIN_LEAK:
        send_alarm_json();
        socketStatus = 0;
        save_socket_status(socketStatus);
        gpio_set_level(PIN_RELAY, socketStatus);
        gpio_set_level(PIN_LED_G, socketStatus);
        break;
    default:
        break;
    }

}

void gpio_Init(void)
{
    // Relay init
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_conf.pin_bit_mask = 1ULL << PIN_RELAY; // Bit mask of the pins
    io_conf.pull_down_en = 0; // Disable pull-down mode
    io_conf.pull_up_en = 0; // Disable pull-up mode
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << PIN_LED_R; // Bit mask of the pins
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << PIN_LED_G; // Bit mask of the pins
    gpio_config(&io_conf);

    gpio_config_t io2_conf = { 
    .mode = GPIO_MODE_INPUT,         
    .pin_bit_mask = 1ULL << PIN_IQR | (1ULL << PIN_LEAK), 
    .pull_down_en = 0,               
    .pull_up_en = 1               
    };
    gpio_config(&io2_conf);

    gpio_set_intr_type(PIN_IQR, GPIO_INTR_NEGEDGE); 
    gpio_set_intr_type(PIN_LEAK, GPIO_INTR_NEGEDGE); 

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // gpio_isr_handler_add(PIN_IQR, gpio_isr_handler, (void*) PIN_IQR);
    // gpio_isr_handler_add(PIN_LEAK, gpio_isr_handler, (void*) PIN_LEAK);
    ESP_LOGI(TAG, "GPIO interrupt configured.");

    gpio_set_level(PIN_RELAY, socketStatus);
    gpio_set_level(PIN_LED_G, socketStatus);
}

bool load_socket_status() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        return false;
    }

    // Read status form NVS
    int8_t status = 0;
    err = nvs_get_i8(my_handle, "socketStatus", &status);
    if (err == ESP_OK) {
        socketStatus = status;
    } else {
        socketStatus = false;  // def if no value
    }

    nvs_close(my_handle);
    return (err == ESP_OK);
}

void save_socket_status(bool status) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        return;  // Không thể mở NVS
    }

    // Lưu trạng thái vào NVS
    int8_t status_int = (status ? 1 : 0);
    err = nvs_set_i8(my_handle, "socketStatus", status_int);
    if (err == ESP_OK) {
        nvs_commit(my_handle);  // Ghi dữ liệu vào bộ nhớ
    }

    nvs_close(my_handle);
}

void socketControl(bool _status)
{   
    if (checkZeroCross())
    {
        socketStatus = _status;
        modeADE7753 = 0;
    }

    save_socket_status(_status);
    gpio_set_level(PIN_RELAY, _status);
    gpio_set_level(PIN_LED_G, _status);
}

bool checkZeroCross(void)
{
    ADE7753_reset();
    ADE7753_setCh1Gain(ADE7753_GAIN_4);
    ADE7753_setCh2Gain(ADE7753_GAIN_2);  
    ADE7753_setCh1FullScale(ADE7753_FS_0_5V);
    ADE7753_setInterrupt(ADE7753_WSMP);
    ADE7753_setWaveSelect(ADE7753_WAV_CH2); // Set channel 2
    ADE7753_setDataRate(ADE7753_DR_1_1024); 

    while (1)
    {
        esp_rom_delay_us(10);
        if (ADE7753_status() && ADE7753_WSMP) {
            VRMS = ADE7753_readWaveForm()/ 34.622;
            if (abs(VRMS) < 20){
                return true;
            }
        }
    }
    
}