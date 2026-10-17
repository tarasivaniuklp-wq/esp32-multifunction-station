#pragma once
#include <stdio.h>
#include <string.h>

#include "Types.hpp"
#include "credentials.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
class NetworkManager {
private:
    QueueHandle_t alarm_cmd_queue = nullptr;
    QueueHandle_t m_settings_queue = nullptr;
    AlarmCmd cmd = AlarmCmd::DISARM;
    AlarmState mod = AlarmState::DISARMED;
    void wifi_init();
    static esp_err_t arm_handler(httpd_req_t* req);
    static esp_err_t disarm_handler(httpd_req_t* req);
    static esp_err_t alarm_page_handler(httpd_req_t* req);
    static esp_err_t index_html_handler(httpd_req_t* req);
    static esp_err_t telemetry_page_handler(httpd_req_t* req);
    static esp_err_t settings_page_handler(httpd_req_t* req);
    static esp_err_t settings_update_handler(httpd_req_t* req);
    void conect_to_server();
    httpd_handle_t server_start();
    httpd_handle_t server = NULL;

public:
    void begin();
    AlarmState get_PIR_state();
    NetworkManager(QueueHandle_t alarm_cmd_queue, QueueHandle_t m_settings_queue);
};