#include "InputManager.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Types.hpp"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN_BACK GPIO_NUM_14
#define BTN_UP GPIO_NUM_27
#define BTN_SELECT GPIO_NUM_26
#define BTN_DOWN GPIO_NUM_25

void InputManager::begin() {
    gpio_reset_pin(BTN_UP);
    gpio_reset_pin(BTN_DOWN);
    gpio_reset_pin(BTN_BACK);
    gpio_reset_pin(BTN_SELECT);

    gpio_set_direction(BTN_UP, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_DOWN, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_BACK, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_SELECT, GPIO_MODE_INPUT);

    gpio_set_pull_mode(BTN_UP, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BTN_DOWN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BTN_BACK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(BTN_SELECT, GPIO_PULLUP_ONLY);

    xTaskCreate(&InputManager::Task_wraper, "InputTask", 2048, this, 5, nullptr);
}
InputManager::InputManager(QueueHandle_t BTN_EV_queue) : m_queue_handle(BTN_EV_queue) {}

void InputManager::run() {
    ButtonId last_btn_state;
    last_btn_state = ButtonId::NONE;
    UIEvent ev;

    while (true) {
        ButtonId cur_btn_state = read_buttons();
        ev.btn_id = cur_btn_state;
        ev.type = UIEventType::BUTTON_PRESS;
        if (cur_btn_state != last_btn_state) {
            if (cur_btn_state != ButtonId::NONE) {
                xQueueSend(m_queue_handle, &ev, 0);
            }
            last_btn_state = cur_btn_state;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void InputManager::Task_wraper(void* func) {
    InputManager* object = static_cast<InputManager*>(func);
    object->run();
}

ButtonId InputManager::read_buttons() {
    if (gpio_get_level(BTN_UP) == 0) return ButtonId::UP;
    if (gpio_get_level(BTN_DOWN) == 0) return ButtonId::DOWN;
    if (gpio_get_level(BTN_SELECT) == 0) return ButtonId::SELECT;
    if (gpio_get_level(BTN_BACK) == 0) return ButtonId::BACK;

    return ButtonId::NONE;
}