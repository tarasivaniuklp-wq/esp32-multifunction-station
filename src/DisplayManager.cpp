#include "DisplayManager.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Types.hpp"
#include "driver/i2c_master.h"
#include "esp_log.h"

void DisplayManager::begin() {
    i2c_master_bus_config_t i2c_cfg;
    memset(&i2c_cfg, 0, sizeof(i2c_cfg));
    i2c_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_cfg.glitch_ignore_cnt = 7;
    i2c_cfg.scl_io_num = GPIO_NUM_22;
    i2c_cfg.sda_io_num = GPIO_NUM_21;
    i2c_cfg.i2c_port = I2C_NUM_0;

    i2c_master_bus_handle_t i2c_handle;

    i2c_new_master_bus(&i2c_cfg, &i2c_handle);

    i2c_device_config_t dev_cfg;
    memset(&dev_cfg, 0, sizeof(dev_cfg));
    dev_cfg.scl_speed_hz = 400000;
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = 0x3C;
    dev_cfg.scl_wait_us = 1000;

    i2c_master_dev_handle_t dev_handle;

    i2c_master_bus_add_device(i2c_handle, &dev_cfg, &dev_handle);

    m_display.begin(dev_handle);
    m_display.oled_update();
    xTaskCreate(&DisplayManager::Task_wraper, "DisplayManager", 4096, this, 4, nullptr);
}
void DisplayManager::Task_wraper(void* param) {
    DisplayManager* object = static_cast<DisplayManager*>(param);
    object->run();
}
void DisplayManager::run() {
    UIEvent ui_ev;
    render();

    while (true) {
        bool needs_render = false;

        if (xQueueReceive(m_telemetry_queue, &data, 0)) {
            ESP_LOGI("DisplayManager", "Data received successfully!");
            telemetry_page.update_data(data);
            if (cur_page == &telemetry_page) {
                needs_render = true;
            }
        }

        if (xQueueReceive(m_ui_ev_queue, &ui_ev, pdMS_TO_TICKS(50))) {
            if (ui_ev.type == UIEventType::BUTTON_PRESS) {
                if (cur_page != nullptr) cur_page->handle_input(ui_ev, *this);
            } else if (ui_ev.type == UIEventType::FORCE_NAVIGATE) {
                if (ui_ev.target_page != nullptr) navigate_to(*ui_ev.target_page);
            } else if (ui_ev.type == UIEventType::REFRESH_SCREEN) {
            }

            needs_render = true;
        }

        if (needs_render) {
            render();
        }
    }
}
void DisplayManager::render() {
    if (cur_page != nullptr) {
        m_display.oled_clear_buffer();
        cur_page->draw(m_display);
        std::string cur_time = TimeManager::get_formated_time("%H:%M");
        m_display.draw_str(84, 2, cur_time.c_str(), true);
        m_display.oled_update();
    }
}
void DisplayManager::navigate_to(IPage& page) { cur_page = &page; }
DisplayManager::DisplayManager(QueueHandle_t display_data_queue_, QueueHandle_t telemetry_queue)
    : m_ui_ev_queue(display_data_queue_), m_telemetry_queue(telemetry_queue) {
    cur_page = &menu_page;
    menu_page.set_target_page(1, &telemetry_page);
    menu_page.set_target_page(0, &alarm_page);
    menu_page.set_target_page(2, &game_page);
    menu_page.set_target_page(3, &s_page);
    telemetry_page.set_target_page(0, &menu_page);
    alarm_page.set_target_page(0, &menu_page);
    game_page.set_target_page(2, &menu_page);
    s_page.set_target_page(3, &menu_page);
};
