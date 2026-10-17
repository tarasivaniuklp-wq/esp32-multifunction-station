#include "TelegramBot.hpp"

static const char* TAG = "[Telegram Bot]";

void TelegramBot::task_wrapper(void* param) {
    auto object = static_cast<TelegramBot*>(param);
    object->run();
}
void TelegramBot::begin() {
    xTaskCreate(&TelegramBot::task_wrapper, "Telegram_task", 8192, this, 2, 0);
    ESP_LOGI(TAG, "Task successfuly created!");
}
void TelegramBot::run() {
    ALarmMessage msg;
    while (true) {
        poll_updates();
        if (m_telegram_queue != nullptr && xQueueReceive(m_telegram_queue, &msg, 100) == pdTRUE) {
            send_message(form_text(msg));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
std::string TelegramBot::form_text(ALarmMessage& alarm) {
    std::string time_str = TimeManager::get_formated_time("%d.%m.%Y %H:%M:%S");
    std::string msg = "<b>⚠️ ТРИВОГА В СИСТЕМІ!</b>\n";
    msg += "<b>Час:</b> <code>" + time_str + "</code>\n";

    switch (alarm.type) {
        case AlarmEvents::MOTION_DETECTED:
            msg += "<b>Подія:</b> 🚶 Виявлено рух PIR-датчиком!";
            break;
        case AlarmEvents::TEMPR_WARN:
            msg += "<b>Подія:</b> 🌡️ Превищення температури! Поточна: " + std::to_string(alarm.data) + "°C";
            break;
        case AlarmEvents::SYSTEM_BOOT:
            msg += "<b>Подія:</b> 🟢 Перезапуск системи ESP32!";
            break;
    }

    return msg;
}
bool TelegramBot::send_message(std::string text) {
    std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";

    std::string payload =
        "{\"chat_id\":" + std::to_string(chat_id) + ",\"text\":\"" + text + "\",\"parse_mode\":\"HTML\"}";
    esp_http_client_config_t config = {};
    memset(&config, 0, sizeof(config));
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.method = HTTP_METHOD_POST;
    config.url = url.c_str();
    config.timeout_ms = 5000;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == nullptr) {
        ESP_LOGI(TAG, "Conection failed");
        return false;
    }
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload.c_str(), payload.length());

    esp_err_t err = esp_http_client_perform(client);
    bool success = false;
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "Message sent successfully!");
            success = true;
        } else {
            ESP_LOGE(TAG, "Telegram API returned HTTP error code: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTPS POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return success;
}
void TelegramBot::poll_updates() {
    std::string url = "https://api.telegram.org/bot" + token +
                      "/getUpdates?offset=" + std::to_string(m_last_update_id) + "&limit=1&timeout=20";
    esp_http_client_config_t config = {};
    memset(&config, 0, sizeof(config));
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 25000;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return;

    if (esp_err_t err = esp_http_client_open(client, 0) == ESP_OK) {
        esp_http_client_fetch_headers(client);
        int status = esp_http_client_get_status_code(client);
        if (status == 200) {
            char buffer[2048];
            int read_ln = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
            if (read_ln > 0) {
                buffer[read_ln] = '\0';
                parse_telegram_json(buffer);
            } else {
                ESP_LOGE(TAG, "getUpdates HTTPS connection failed: %s", esp_err_to_name(err));
            }
        }
    }
    esp_http_client_cleanup(client);
}
void TelegramBot::process_command(int64_t chat_id, const std::string& text) {
    bool allowed = false;
    for (size_t i = 0; i < allowed_ids_count; i++) {
        if (chat_id == allowed_chat_ids[i]) {
            allowed = true;
        }
    }
    if (allowed) {
        ESP_LOGI(TAG, "Allowed user_id!");
    } else {
        ESP_LOGW(TAG, "Unallowed access attempt from ID: %lld", chat_id);
        return;
    }

    std::string command = text;
    std::string args = "";

    size_t space_pos = text.find(' ');
    if (space_pos != std::string::npos) {
        command = text.substr(0, space_pos);
        args = text.substr(space_pos + 1);
    }

    ESP_LOGI(TAG, "Command: %s | Args: %s", command.c_str(), args.c_str());

    if (command == "/start" || command == "/help") {
        std::string reply = "<b>🤖 ESP32 Control Hub</b>\n\n";
        reply += "Доступні команди:\n";
        reply += "• /status - Стан системи та датчиків\n";
        reply += "• /time - Поточний синхронізований час\n";
        reply += "• /alarm [on/off] - Керування охороною\n";
        reply += "• /page_back - Переключити сторінку на OLED";

        send_message(reply);
    }

    else if (command == "/status") {
        std::string time_str = TimeManager::get_formated_time("%d.%m.%Y %H:%M:%S");

        wifi_ap_record_t ap_info;
        int rssi_val = 0;

        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            rssi_val = ap_info.rssi;
        }

        std::string reply = "<b>📊 СТАТУС СИСТЕМИ:</b>\n";
        reply += "<b>Час:</b> <code>" + time_str + "</code>\n";
        reply += "<b>Wi-Fi RSSI:</b> " + std::to_string(rssi_val) + " dBm\n";
        reply += "<b>Free Heap:</b> " + std::to_string(esp_get_free_heap_size() / 1024) + " KB";
        send_message(reply);
    }

    else if (command == "/time") {
        std::string time_str = TimeManager::get_formated_time("%H:%M:%S (%Z)");
        send_message("<b>🕒 Системний час:</b> <code>" + time_str + "</code>");
    }

    else if (command == "/alarm") {
        AlarmCmd cmd;
        if (args == "on") {
            cmd = AlarmCmd::ARM;
            xQueueSend(alarm_cmd_queue, &cmd, 100);
            send_message("<b>🛡️ Охорону УВІМКНЕНО!</b>");
        } else if (args == "off") {
            cmd = AlarmCmd::DISARM;
            xQueueSend(alarm_cmd_queue, &cmd, 100);
            send_message("<b>🔓 Охорону ВИМКНЕНО!</b>");
        } else {
            send_message("❓ Використання: <code>/alarm on</code> або <code>/alarm off</code>");
        }
    }

    else if (command == "/page_back") {
        if (ui_queue != nullptr) {
            UIEvent ev;
            ev.btn_id = ButtonId::BACK;
            ev.type = UIEventType::BUTTON_PRESS;
            xQueueSend(ui_queue, &ev, 0);

            send_message("📺 Сторінку на OLED-дисплеї змінено.");
        }
    }

    else {
        send_message("❌ Невідома команда! Напишіть /help");
    }
}
void TelegramBot::parse_telegram_json(const char* json_str) {
    cJSON* root = cJSON_Parse(json_str);
    if (root == nullptr) {
        ESP_LOGW(TAG, "Failed to parse JSON!");
        return;
    }
    cJSON* ok = cJSON_GetObjectItem(root, "ok");
    if (cJSON_IsTrue(ok)) {
        cJSON* result_arr = cJSON_GetObjectItem(root, "result");
        if (cJSON_IsArray(result_arr)) {
            int arr_size = cJSON_GetArraySize(result_arr);
            for (int i = 0; i < arr_size; i++) {
                cJSON* object = cJSON_GetArrayItem(result_arr, i);

                cJSON* update_id_node = cJSON_GetObjectItem(object, "update_id");
                if (update_id_node != nullptr) {
                    m_last_update_id = update_id_node->valueint + 1;
                }

                cJSON* message = cJSON_GetObjectItem(object, "message");
                if (message != nullptr) {
                    cJSON* text = cJSON_GetObjectItem(message, "text");
                    cJSON* chat = cJSON_GetObjectItem(message, "chat");
                    cJSON* chat_id = nullptr;
                    if (chat != nullptr) {
                        chat_id = cJSON_GetObjectItem(chat, "id");
                    }
                    if (text->valuestring != nullptr && cJSON_IsString(text) && chat_id != nullptr &&
                        cJSON_IsNumber(chat_id)) {
                        std::string command = text->valuestring;
                        int64_t sender = chat_id->valueint;
                        process_command(sender, command);
                    }
                }
            }
            cJSON_Delete(root);
        }
    }
}
TelegramBot::TelegramBot(std::string token_, int64_t chat_id_, const int64_t* allowed_ids_, size_t count_,
                         QueueHandle_t m_telegram_queue_, QueueHandle_t alarm_cmd_queue_, QueueHandle_t ui_queue_)
    : token(token_),
      chat_id(chat_id_),
      allowed_chat_ids(allowed_ids_),
      allowed_ids_count(count_),
      m_telegram_queue(m_telegram_queue_),
      alarm_cmd_queue(alarm_cmd_queue_),
      ui_queue(ui_queue_) {};