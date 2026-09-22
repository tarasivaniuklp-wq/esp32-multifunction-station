#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Types.hpp"

class InputManager {
public:
    InputManager(QueueHandle_t BTN_EV_queue);
    void begin();

private:
    QueueHandle_t m_queue_handle = nullptr;
    static void Task_wraper(void* func);
    void run();
    ButtonId read_buttons();
};