#include "NetworkManager.hpp"

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
    const char* html_page = R"raw(
<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Розумний Дім</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f7f6; color: #333; text-align: center; margin: 0; padding: 20px; }
        h1 { color: #2c3e50; margin-bottom: 30px; }
        .menu { display: flex; flex-direction: column; gap: 15px; max-width: 300px; margin: 0 auto; }
        .btn { padding: 15px 20px; font-size: 18px; font-weight: bold; color: white; background-color: #3498db; text-decoration: none; border-radius: 8px; transition: background 0.3s, transform 0.1s; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        .btn:hover { background-color: #2980b9; }
        .btn:active { transform: scale(0.98); }
        .btn.alarm { background-color: #e74c3c; }
        .btn.alarm:hover { background-color: #c0392b; }
        .btn.settings { background-color: #7f8c8d; }
        .btn.settings:hover { background-color: #636e72; }
    </style>
</head>
<body>
    <h1>Панель керування ESP32</h1>
    <div class="menu">
        <a href="/alarm" class="btn alarm">🚨 Сигналізація</a>
        <a href="/telemetry" class="btn">🌡 Телеметрія</a>
        <a href="/settings" class="btn settings">⚙️ Налаштування</a>
    </div>
</body>
</html>
)raw";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t NetworkManager::telemetry_page_handler(httpd_req_t* req) {
    char page[2048] = "";
    httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
esp_err_t NetworkManager::settings_page_handler(httpd_req_t* req) {
    char page[2048] = "";
    httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
esp_err_t NetworkManager::arm_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);
    self->cmd = AlarmCmd::ARM;
    self->mod = AlarmState::ARMED;

    xQueueSend(self->alarm_cmd_queue, &self->cmd, 100);
    const char* resp = "<h1>Сигналізацію поставлено на охорону!</h1><p><a href='/'><button>Назад</button></a></p>";
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t NetworkManager::disarm_handler(httpd_req_t* req) {
    auto self = static_cast<NetworkManager*>(req->user_ctx);
    self->cmd = AlarmCmd::DISARM;
    self->mod = AlarmState::DISARMED;
    xQueueSend(self->alarm_cmd_queue, &self->cmd, 100);
    const char* resp = "<h1>Сигналізацію знято з охорони!</h1><p><a href='/'><button>Назад</button></a></p>";
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
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
    httpd_register_uri_handler(server, &index_html_uri);
    httpd_register_uri_handler(server, &alarm_uri);
    httpd_register_uri_handler(server, &telemetry_uri);
    httpd_register_uri_handler(server, &settings_uri);
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
NetworkManager::NetworkManager(QueueHandle_t alarm_cmd_queue_) : alarm_cmd_queue(alarm_cmd_queue_) {};
void NetworkManager::begin() {
    wifi_init();
    server_start();
    ESP_LOGI("NetworkManager", "wifi successfuly inited, server was started!");
}