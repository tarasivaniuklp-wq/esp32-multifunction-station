#include "SensorManager.hpp"

static const char* TAG = "[SensorManager]";
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

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    if (!wait_for_level(0, 500) || !wait_for_level(1, 500) || !wait_for_level(0, 500)) {
        portEXIT_CRITICAL(&mux);
        ESP_LOGW(TAG, "Timeout during DHT start sequence! (Sensor didn't respond)");
        return false;
    }

    uint8_t data[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 40; i++) {
        if (!wait_for_level(1, 100)) {
            portEXIT_CRITICAL(&mux);
            ESP_LOGW(TAG, "Timeout waiting for bit %d to go HIGH!", i);
            return false;
        }
        uint64_t start_time = esp_timer_get_time();
        if (!wait_for_level(0, 500)) {
            portEXIT_CRITICAL(&mux);
            ESP_LOGW(TAG, "Timeout waiting for bit %d to go LOW!", i);
            return false;
        }
        uint64_t pulse_time = esp_timer_get_time() - start_time;
        data[i / 8] <<= 1;
        if (pulse_time > 50) {
            data[i / 8] |= 1;
        }
    }

    portEXIT_CRITICAL(&mux);
    ESP_LOGI(TAG, "RAW BYTES: %d %d %d %d | SUM: %d", data[0], data[1], data[2], data[3], data[4]);
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
    data.humidity = last_hum;
    data.temp = last_temp;

    data.motion_detected = read_info_PIR();

    static bool last_motion_state = false;

    if (data.motion_detected && !last_motion_state) {
        ESP_LOGW(TAG, "PIR Sensor triggered! Sending to queue...");
        xQueueSend(telemetry_queue, &data, 100);
        xQueueSend(display_data_queue, &data, 100);
    }
    last_motion_state = data.motion_detected;

    return data.motion_detected;
}

void SensorManager::TaskWrapper(void* param) {
    SensorManager* object = static_cast<SensorManager*>(param);
    object->run();
}
void SensorManager::run() {
    ESP_LOGI(TAG, "Process successfully launched!");

    uint32_t loop_counter = 0;

    while (true) {
        // 1. Опитуємо PIR-датчик дуже швидко (кожні 50 мс)
        read_info();

        // 2. Опитуємо DHT11 лише раз на 2000 мс (кожні 40 циклів по 50 мс)
        // Коли полагодиш DHT11, просто розкоментуєш цей блок:
        /*
        if (loop_counter >= 40) {
            float hum, temp;
            if (read_info_DHT11(&hum, &temp)) {
                last_hum = hum;
                last_temp = temp;
                // Відправляємо нову температуру на екран
                SensorData env_data = {hum, temp, false};
                xQueueSend(display_data_queue, &env_data, 100);
            }
            loop_counter = 0;
        }
        */

        loop_counter++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
void SensorManager::begin() {
    gpio_reset_pin(PIR_PIN);
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    dht_pin_init();
    xTaskCreate(&SensorManager::TaskWrapper, "SensorTask", 2048, this, 6, nullptr);
}
SensorManager::SensorManager(QueueHandle_t telemetry_queue_, QueueHandle_t display_data_queue_,
                             QueueHandle_t telegram_queue_)
    : telemetry_queue(telemetry_queue_), display_data_queue(display_data_queue_), telegram_queue(telegram_queue_) {};