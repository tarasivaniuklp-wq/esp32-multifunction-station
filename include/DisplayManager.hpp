#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "OLED_display.hpp"
#include "Pages.hpp"
#include "Types.hpp"
#include "esp_log.h"

class DisplayManager {
public:
    DisplayManager(QueueHandle_t display_data_queue, QueueHandle_t telemetry_queue);
    void begin();
    IPage* get_cur_page();
    void set_page();
    void navigate_to(IPage& page);
    void navigate_back(IPage& page);

private:
    SensorData data;
    OLED_display m_display;
    QueueHandle_t m_ui_ev_queue = nullptr;
    QueueHandle_t m_telemetry_queue = nullptr;

    MenuPage menu_page;
    AlarmPage alarm_page;
    GamePage game_page;
    SettingsPage s_page;
    TelemetryPage telemetry_page;

    IPage* cur_page = &menu_page;
    void render();
    void run();
    static void Task_wraper(void* param);
};