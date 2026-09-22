#include <stdio.h>

#include "Types.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define BUZZER_PIN GPIO_NUM_26

class AlarmControler {
public:
    void run();
    static void Task_wrapper(void* param);
    void begin();
    AlarmControler(QueueHandle_t m_alarm_cmd_queue_, QueueHandle_t m_telemetry_queue_, QueueHandle_t m_ui_queue_,
                   IPage* triger_page_)
        : m_alarm_cmd_queue(m_alarm_cmd_queue_),
          m_telemetry_queue(m_telemetry_queue_),
          m_ui_queue(m_ui_queue_),
          triger_page(triger_page_) {};
    AlarmState get_state();

private:
    QueueHandle_t m_alarm_cmd_queue = nullptr;
    QueueHandle_t m_telemetry_queue = nullptr;
    QueueHandle_t m_ui_queue = nullptr;
    AlarmState m_state = AlarmState::DISARMED;
    IPage* triger_page = nullptr;
};