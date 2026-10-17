#pragma once

#include <cJSON.h>

#include <string>

#include "TimeManager.hpp"
#include "Types.hpp"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
class TelegramBot {
private:
    std::string token;
    int64_t chat_id;
    const int64_t* allowed_chat_ids;
    size_t allowed_ids_count;
    QueueHandle_t m_telegram_queue;
    QueueHandle_t alarm_cmd_queue;
    QueueHandle_t ui_queue;
    int m_last_update_id;

    void run();
    static void task_wrapper(void* param);
    std::string form_text(ALarmMessage& alarm);

    void process_command(int64_t chat_id, const std::string& text);
    void poll_updates();
    void parse_telegram_json(const char* json_str);

public:
    void begin();
    bool send_message(std::string text);
    TelegramBot(std::string token_, int64_t chat_id_, const int64_t* allowed_ids_, size_t count_,
                QueueHandle_t m_telegram_queue_, QueueHandle_t alarm_cmd_queue_, QueueHandle_t ui_queue);
};
