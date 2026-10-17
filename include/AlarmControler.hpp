#include <stdio.h>

#include "Types.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define BUZZER_PIN GPIO_NUM_5

class AlarmControler {
public:
    void run();
    static void Task_wrapper(void* param);
    void begin();
    AlarmControler(QueueHandle_t m_alarm_cmd_queue_, QueueHandle_t m_telemetry_queue_, QueueHandle_t m_setting_queue_,
                   QueueHandle_t m_ui_queue_, IPage* triger_page_, QueueHandle_t telegrem_queue_)
        : m_alarm_cmd_queue(m_alarm_cmd_queue_),
          m_telemetry_queue(m_telemetry_queue_),
          m_settings_queue(m_setting_queue_),
          m_ui_queue(m_ui_queue_),
          triger_page(triger_page_),
          telegrem_queue(telegrem_queue_) {};
    AlarmState get_state();
    void send_command(AlarmCmd cmd);
    uint8_t get_timer() { return m_arming_countdown; }

private:
    QueueHandle_t m_alarm_cmd_queue = nullptr;
    QueueHandle_t m_telemetry_queue = nullptr;
    QueueHandle_t m_settings_queue = nullptr;
    QueueHandle_t m_ui_queue = nullptr;
    IPage* triger_page = nullptr;
    QueueHandle_t telegrem_queue = nullptr;
    AlarmState m_state = AlarmState::DISARMED;
    SystemSettings m_cur_settings = {10, true, true};
    uint8_t m_arming_countdown = 0;
    uint32_t m_last_tick = 0;
};