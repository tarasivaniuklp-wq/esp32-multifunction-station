#include "NetworkManager.hpp"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

extern const uint8_t telemetry_html_start[] asm("_binary_telemetry_html_start");
extern const uint8_t telemetry_html_end[] asm("_binary_telemetry_html_end");

extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_settings_html_end");

void NetworkManager::wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_cfg = {};

    snprintf((char*)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid), "%s", WIFI_SSID);
    snprintf((char*)wifi_cfg.sta.password, sizeof(wifi_cfg.sta.password), "%s", WIFI_PASS);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);

    esp_wifi_start();
    esp_wifi_connect();
}
esp_err_t NetworkManager::index_html_handler(httpd_req_t* req) {
    const size_t index_html_size = (index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, (const char*)index_html_start, index_html_size);
    return ESP_OK;
}

esp_err_t NetworkManager::telemetry_page_handler(httpd_req_t* req) {
    size_t size = telemetry_html_end - telemetry_html_start;
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, (const char*)telemetry_html_start, size);
    return ESP_OK;
}
esp_err_t NetworkManager::settings_page_handler(httpd_req_t* req) {
    size_t size = settings_html_end - settings_html_start;
    httpd_resp_set_type(req, "text/html, charset = utf-8");
    httpd_resp_send(req, (const char*)settings_html_start, size);
    return ESP_OK;
}
esp_err_t NetworkManager::settings_update_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);

    SystemSettings new_settings = {10, true, true};

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char buf[buf_len];
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[10];
            if (httpd_query_key_value(buf, "delay", param, sizeof(param)) == ESP_OK) {
                new_settings.exit_delay_sec = atoi(param);
            }
            if (httpd_query_key_value(buf, "buzzer", param, sizeof(param)) == ESP_OK) {
                new_settings.siren_enabled = true;
            } else {
                new_settings.siren_enabled = false;
            }
            if (httpd_query_key_value(buf, "tg_notif", param, sizeof(param))) {
                new_settings.tg_notifications = true;
            } else {
                new_settings.tg_notifications = false;
            }
        }
    }

    xQueueSend(self->m_settings_queue, &new_settings, 100);

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}
esp_err_t NetworkManager::arm_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);
    self->cmd = AlarmCmd::ARM;
    self->mod = AlarmState::ARMED;

    xQueueSend(self->alarm_cmd_queue, &self->cmd, 100);

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/alarm");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t NetworkManager::disarm_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);
    self->cmd = AlarmCmd::DISARM;
    self->mod = AlarmState::DISARMED;

    xQueueSend(self->alarm_cmd_queue, &self->cmd, 100);

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/alarm");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t NetworkManager::alarm_page_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);
    const char* status_str = "ЗНЯТО З ОХОРОНИ";
    if (self->mod == AlarmState::ARMED) {
        status_str = "НА ОХОРОНІ";
    } else if (self->mod == AlarmState::TRIGGERED) {
        status_str = "УВАГА: ТРИВОГА!";
    }

    char html_page[512];
    snprintf(html_page, sizeof(html_page),
             "<!DOCTYPE html><html><head><meta charset='utf-8'></head><body>"
             "<h1>Система Охорони ESP32</h1>"
             "<p>Стан: <b>%s</b></p>"
             "<a href='/alarm/arm'><button>Поставити на охорону</button></a> "
             "<a href='/alarm/disarm'><button>Зняти з охорони</button></a>"
             "</body></html>",
             status_str);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
void NetworkManager::conect_to_server() {
    httpd_uri_t index_html_uri = {.uri = "/", .method = HTTP_GET, .handler = index_html_handler, .user_ctx = this};
    httpd_uri_t alarm_uri = {.uri = "/alarm", .method = HTTP_GET, .handler = alarm_page_handler, .user_ctx = this};
    httpd_uri_t telemetry_uri = {
        .uri = "/telemetry", .method = HTTP_GET, .handler = telemetry_page_handler, .user_ctx = this};
    httpd_uri_t settings_uri = {
        .uri = "/settings", .method = HTTP_GET, .handler = settings_page_handler, .user_ctx = this};
    httpd_uri_t settings_update_uri = {
        .uri = "/settings/update", .method = HTTP_GET, .handler = settings_update_handler, .user_ctx = this};
    httpd_uri_t arm_uri = {.uri = "/alarm/arm", .method = HTTP_GET, .handler = arm_handler, .user_ctx = this};
    httpd_uri_t disarm_uri = {.uri = "/alarm/disarm", .method = HTTP_GET, .handler = disarm_handler, .user_ctx = this};

    httpd_register_uri_handler(server, &index_html_uri);
    httpd_register_uri_handler(server, &alarm_uri);
    httpd_register_uri_handler(server, &telemetry_uri);
    httpd_register_uri_handler(server, &settings_uri);
    httpd_register_uri_handler(server, &settings_update_uri);
    httpd_register_uri_handler(server, &arm_uri);
    httpd_register_uri_handler(server, &disarm_uri);
}
httpd_handle_t NetworkManager::server_start() {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &cfg) == ESP_OK) {
        conect_to_server();
        return server;
    }
    return NULL;
}
AlarmState NetworkManager::get_PIR_state() { return mod; }
NetworkManager::NetworkManager(QueueHandle_t alarm_cmd_queue_, QueueHandle_t m_settings_queue_)
    : alarm_cmd_queue(alarm_cmd_queue_), m_settings_queue(m_settings_queue_) {};
void NetworkManager::begin() {
    wifi_init();
    server_start();
    ESP_LOGI("NetworkManager", "wifi successfuly inited, server was started!");
}