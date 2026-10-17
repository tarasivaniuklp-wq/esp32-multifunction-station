#include <stdio.h>

#include "AlarmControler.hpp"
#include "DisplayManager.hpp"
#include "InputManager.hpp"
#include "NetworkManager.hpp"
#include "SensorManager.hpp"
#include "TelegramBot.hpp"
#include "Types.hpp"
#include "credentials.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
// продефайнити піни

static const char* INIT_TAG = "[INIT LOG]";

QueueHandle_t g_ui_queue = nullptr;
QueueHandle_t g_telemetry_queue = nullptr;
QueueHandle_t g_telegram_queue = nullptr;
QueueHandle_t g_alarm_cmd_queue = nullptr;
QueueHandle_t g_display_data_queue = nullptr;
QueueHandle_t g_settings_queue = nullptr;

void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(INIT_TAG, "NVS Initialized");
}
extern "C" void app_main() {
    nvs_init();
    g_display_data_queue = xQueueCreate(10, sizeof(SensorData));
    g_ui_queue = xQueueCreate(10, sizeof(UIEvent));
    g_telemetry_queue = xQueueCreate(10, sizeof(SensorData));
    g_telegram_queue = xQueueCreate(10, sizeof(ALarmMessage));
    g_alarm_cmd_queue = xQueueCreate(10, sizeof(AlarmCmd));
    g_settings_queue = xQueueCreate(10, sizeof(SystemSettings));

    if (g_ui_queue == nullptr || g_telemetry_queue == nullptr || g_telegram_queue == nullptr) {
        ESP_LOGE(INIT_TAG, "Failed to create queues!");
        return;
    }
    ESP_LOGI(INIT_TAG, "System core ready. Starting modules...");

    static InputManager input_mng(g_ui_queue);
    static DisplayManager display_mng(g_ui_queue, g_telemetry_queue);
    static NetworkManager network_mng(g_alarm_cmd_queue, g_settings_queue);
    static SensorManager sensor_mng(g_telemetry_queue, g_display_data_queue, g_telegram_queue);
    static AlarmControler alarm_cntr(g_alarm_cmd_queue, g_telemetry_queue, g_settings_queue, g_ui_queue,
                                     display_mng.get_alarm_page(), g_telegram_queue);
    static TelegramBot tg_bot(TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, allowed_chats_id,
                              sizeof(allowed_chats_id) / sizeof(allowed_chats_id[0]), g_telegram_queue,
                              g_alarm_cmd_queue, g_ui_queue);

    display_mng.get_alarm_page()->set_contoler(&alarm_cntr);
    vTaskDelay(pdMS_TO_TICKS(5000));

    network_mng.begin();

    TimeManager::init_sntp();
    TimeManager::wait_for_sync(20000);

    display_mng.begin();
    input_mng.begin();
    sensor_mng.begin();
    alarm_cntr.begin();
    tg_bot.begin();
}
