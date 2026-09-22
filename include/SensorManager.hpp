#pragma once
#include <esp_timer.h>
#include <rom/ets_sys.h>
#include <string.h>

#include "Types.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define DHT_PIN GPIO_NUM_23
#define PIR_PIN GPIO_NUM_27
// Вписати справжні піни

class SensorManager {
private:
    QueueHandle_t telemetry_queue;
    QueueHandle_t display_data_queue;
    float last_temp = 0;
    float last_hum = 0;
    void dht_pin_init();
    void pir_pin_init();
    bool wait_for_level(int target_level, uint32_t timeout);
    bool read_info_DHT11(float* humidity, float* temperature);
    bool read_info_PIR();
    void run();
    static void TaskWrapper(void* param);

public:
    void begin();
    bool read_info();
    void buzzer_on();
    void buzzer_off();
    SensorManager(QueueHandle_t telemetry_queue_, QueueHandle_t display_data_queue_);
};
