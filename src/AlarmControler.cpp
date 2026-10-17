#include "AlarmControler.hpp"
static const char* TAG = "[AlarmControler]";
void AlarmControler::Task_wrapper(void* param) {
    AlarmControler* object = static_cast<AlarmControler*>(param);
    object->run();
}
void AlarmControler::begin() {
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);
    xTaskCreate(&AlarmControler::Task_wrapper, "AlarmControler", 2048, this, 1, nullptr);
}
AlarmState AlarmControler::get_state() { return m_state; }
void AlarmControler::send_command(AlarmCmd cmd) { xQueueSend(m_alarm_cmd_queue, &cmd, 1000); }
void AlarmControler::run() {
    AlarmCmd cmd;
    SensorData data;
    UIEvent ev;
    ALarmMessage alarm_msg;
    UIEvent refresh_ev;
    refresh_ev.type = UIEventType::REFRESH_SCREEN;
    SystemSettings new_settings;
    while (true) {
        if (xQueueReceive(m_settings_queue, &new_settings, 10) == pdTRUE) m_cur_settings = new_settings;

        if (m_state == AlarmState::ARMED && m_arming_countdown > 0) {
            if (xTaskGetTickCount() - m_last_tick >= pdMS_TO_TICKS(1000)) {
                m_arming_countdown--;
                m_last_tick = xTaskGetTickCount();
                ESP_LOGI(TAG, "Таймер: %d сек", m_arming_countdown);

                xQueueSend(m_ui_queue, &refresh_ev, 0);

                if (m_arming_countdown == 0) {
                    ESP_LOGI(TAG, "Час вийшов! Систему повністю АКТИВОВАНО.");
                }
            }
        }

        if (xQueueReceive(m_telemetry_queue, &data, 10) == pdTRUE) {
            if (data.motion_detected && m_state == AlarmState::ARMED && m_arming_countdown == 0) {
                if (m_cur_settings.siren_enabled) gpio_set_level(BUZZER_PIN, 1);
                m_state = AlarmState::TRIGGERED;
                ev.target_page = triger_page;
                ev.type = UIEventType::FORCE_NAVIGATE;
                alarm_msg.type = AlarmEvents::MOTION_DETECTED;
                xQueueSend(m_ui_queue, &ev, 1000);
                if (m_cur_settings.tg_notifications) xQueueSend(telegrem_queue, &alarm_msg, 1000);
            }
        }

        if (xQueueReceive(m_alarm_cmd_queue, &cmd, 10)) {
            if (cmd == AlarmCmd::DISARM || cmd == AlarmCmd::RESET_TRIGGER) {
                m_state = AlarmState::DISARMED;
                m_arming_countdown = 0;
                gpio_set_level(BUZZER_PIN, 0);
                ESP_LOGI(TAG, "Систему ЗНЯТО з охорони / Тривогу скинуто");
                xQueueSend(m_ui_queue, &refresh_ev, 0);
            } else if (m_state == AlarmState::DISARMED && cmd == AlarmCmd::ARM) {
                m_state = AlarmState::ARMED;
                m_arming_countdown = 10;
                m_last_tick = xTaskGetTickCount();
                ESP_LOGI(TAG, "ЗАПУСК ОХОРОНИ: у вас є 10 секунд, щоб вийти!");
                xQueueSend(m_ui_queue, &refresh_ev, 0);
            }
        }
    }
}