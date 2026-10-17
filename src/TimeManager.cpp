#include "TimeManager.hpp"

static const char* TAG = "TIME_LOG";

void TimeManager::notifier(struct timeval* tv) { ESP_LOGI(TAG, "SNTP event triggered!"); }

void TimeManager::init_sntp() {
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);

    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.windows.com");
    esp_sntp_setservername(2, "time.google.com");

    sntp_set_time_sync_notification_cb(notifier);
    esp_sntp_init();

    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/3", 1);
    tzset();
}

bool TimeManager::wait_for_sync(uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "Successfully synced!");
            return true;
        }

        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year > (2020 - 1900)) {
            ESP_LOGI(TAG, "Successfully synced (verified by year)!");
            return true;
        }

        ESP_LOGI(TAG, "Waiting for sync...");
        vTaskDelay(pdMS_TO_TICKS(500));
        elapsed += 500;
    }

    ESP_LOGW(TAG, "Timeout! SNTP failed to sync.");
    return false;
}

std::string TimeManager::get_formated_time(const char* format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);

    return std::string(buffer);
}