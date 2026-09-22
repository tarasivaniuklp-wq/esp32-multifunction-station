#include "AlarmControler.hpp"

void AlarmControler::Task_wrapper(void* param) {
    AlarmControler* object = static_cast<AlarmControler*>(param);
    object->run();
}
void AlarmControler::begin() { xTaskCreate(&AlarmControler::Task_wrapper, "AlarmControler", 2048, this, 1, nullptr); }
AlarmState AlarmControler::get_state() { return m_state; }
void AlarmControler::run() {
    AlarmCmd cmd;
    SensorData data;
    UIEvent ev;
    while (true) {
        if (xQueueReceive(m_telemetry_queue, &data, 100)) {
            if (data.motion_detected && m_state == AlarmState::ARMED) {
                gpio_set_level(BUZZER_PIN, 1);
                m_state = AlarmState::TRIGGERED;
                ev.target_page = triger_page;
                ev.type = UIEventType::FORCE_NAVIGATE;
                xQueueSend(m_ui_queue, &ev, 1000);
            }
        }
        if (xQueueReceive(m_alarm_cmd_queue, &cmd, 100)) {
            if (m_state == AlarmState::ARMED && cmd == AlarmCmd::DISARM) {
                m_state = AlarmState::DISARMED;
            } else if (m_state == AlarmState::DISARMED && cmd == AlarmCmd::ARM) {
                m_state = AlarmState::ARMED;
            } else if (m_state == AlarmState::TRIGGERED && cmd == AlarmCmd::RESET_TRIGGER) {
                m_state = AlarmState::DISARMED;
                gpio_set_level(BUZZER_PIN, 0);
            }
        }
    }
}