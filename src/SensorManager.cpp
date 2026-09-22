#include "SensorManager.hpp"

void SensorManager::dht_pin_init() {
    gpio_config_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    conf.pin_bit_mask = (1ULL << DHT_PIN);
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&conf);
    gpio_set_level(DHT_PIN, 1);
}
bool SensorManager::wait_for_level(int target_level, uint32_t timeout) {
    uint64_t start_time = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) != target_level) {
        if (esp_timer_get_time() - start_time >= timeout) {
            return false;
        }
    }
    return true;
}
bool SensorManager::read_info_DHT11(float* humidity, float* temperature) {
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    if (!wait_for_level(0, 100) || !wait_for_level(1, 100) || !wait_for_level(0, 100)) {
        portEXIT_CRITICAL(&mux);
        return false;
    }

    uint8_t data[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 40; i++) {
        if (!wait_for_level(1, 100)) {
            portEXIT_CRITICAL(&mux);
            return false;
        }
        uint64_t start_time = esp_timer_get_time();
        if (!wait_for_level(0, 100)) {
            portEXIT_CRITICAL(&mux);
            return false;
        }
        uint64_t pulse_time = esp_timer_get_time() - start_time;
        data[i / 8] <<= 1;
        if (pulse_time > 40) {
            data[i / 8] |= 1;
        }
    }

    portEXIT_CRITICAL(&mux);

    // 3. Перевірка контрольної суми
    if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4]) return false;

    // Декодування
    *humidity = (float)data[0] + (float)data[1] * 0.1f;

    *temperature = (float)data[2] + (float)(data[3] & 0x7F) * 0.1f;
    if (data[3] & 0x80) {
        *temperature *= -1.0f;
    }

    return true;
}
bool SensorManager::read_info_PIR() { return gpio_get_level(PIR_PIN); }
bool SensorManager::read_info() {
    SensorData data;
    bool DHT11_info = read_info_DHT11(&data.humidity, &data.temp);
    data.motion_detected = read_info_PIR();
    if (last_hum != data.humidity || last_temp != data.temp || data.motion_detected) {
        xQueueSend(telemetry_queue, &data, 0);
        xQueueSend(display_data_queue, &data, 0);
    }
    return DHT11_info || data.motion_detected;
}

void SensorManager::TaskWrapper(void* param) {
    SensorManager* object = static_cast<SensorManager*>(param);
    object->run();
}
void SensorManager::run() {
    while (true) {
        uint8_t err_cnt = 0;
        if (!read_info()) {
            err_cnt++;
        } else {
            err_cnt = 0;
        }
        if (err_cnt >= 5) {
            err_cnt = 0;
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void SensorManager::begin() {
    dht_pin_init();
    xTaskCreate(&SensorManager::TaskWrapper, "SensorTask", 2048, this, 6, nullptr);
}
SensorManager::SensorManager(QueueHandle_t telemetry_queue_, QueueHandle_t display_data_queue_)
    : telemetry_queue(telemetry_queue_), display_data_queue(display_data_queue_) {}