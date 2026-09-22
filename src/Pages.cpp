
#include "Pages.hpp"

#include "AlarmControler.hpp"
#include "DisplayManager.hpp"
#include "Types.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void draw_home_icon(OLED_display& display, int16_t x, int16_t y) {
    display.draw_line(x + 3, y, x, y + 3, 1);
    display.draw_line(x + 3, y, x + 6, y + 3, 1);
    display.draw_line(x + 1, y + 3, x + 5, y + 3, 1);
    display.draw_line(x + 1, y + 4, x + 1, y + 7, 1);
    display.draw_line(x + 5, y + 4, x + 5, y + 7, 1);
    display.draw_line(x + 1, y + 7, x + 5, y + 7, 1);
    display.draw_pixel(x + 3, y + 6, 1);
}
MenuPage::MenuPage() {
    menu_list[0] = {"Alarm", nullptr, nullptr};
    menu_list[1] = {"Temp&Hum", nullptr, nullptr};
    menu_list[2] = {"Games", nullptr, nullptr};
    menu_list[3] = {"Settings", nullptr, nullptr};
}
void MenuPage::set_target_page(size_t index, IPage* target) {
    if (index < options_cnt) {
        menu_list[index].target_page = target;
    }
}
void MenuPage::draw(OLED_display& display) {
    draw_home_icon(display, 3, 2);
    display.draw_str(15, 2, "Home", 1);
    display.draw_line(78, 0, 78, 11, 1);

    display.draw_str(84, 2, "14:02", true);

    display.draw_line(0, 11, 127, 11, 1);

    const int16_t item_height = 13;
    const int16_t start_y = 12;

    for (size_t i = 0; i < options_cnt; ++i) {
        int16_t y = start_y + (i * item_height);
        bool is_selected = (i == selected_index);

        if (is_selected) {
            display.fill_rect(0, y, 128, item_height, 1);

            display.draw_str(8, y + 2, menu_list[i].title, 0);
        } else {
            display.draw_str(8, y + 2, menu_list[i].title, 1);
        }

        if (i < options_cnt - 1) {
            display.draw_line(0, y + item_height - 1, 127, y + item_height - 1, 1);
        }
    }
}
void MenuPage::handle_input(UIEvent btn_ev, DisplayManager& display_mng) {
    switch (btn_ev.btn_id) {
        case ButtonId::DOWN:
            selected_index = (selected_index + 1) % options_cnt;
            break;

        case ButtonId::UP:
            selected_index = (selected_index == 0) ? (options_cnt - 1) : (selected_index - 1);
            break;

        case ButtonId::SELECT:
            if (menu_list[selected_index].target_page != nullptr) {
                display_mng.navigate_to(*menu_list[selected_index].target_page);
            }
            break;

        case ButtonId::BACK:
            ESP_LOGI("INFO", "Not valid button in this menu!");
            break;

        default:
            break;
    }
}
// Сторінка з Даними датчиків
void draw_thermometer_icon(OLED_display& display, int16_t x, int16_t y) {
    display.draw_line(x + 2, y, x + 4, y, 1);
    display.draw_line(x + 2, y + 1, x + 2, y + 7, 1);
    display.draw_line(x + 4, y + 1, x + 4, y + 7, 1);

    display.draw_line(x + 1, y + 7, x + 5, y + 7, 1);
    display.draw_pixel(x, y + 8, 1);
    display.draw_pixel(x + 6, y + 8, 1);
    display.draw_line(x + 1, y + 9, x + 5, y + 9, 1);

    display.fill_rect(x + 1, y + 8, 5, 2, 1);
    display.draw_line(x + 3, y + 3, x + 3, y + 7, 1);
}
TelemetryPage::TelemetryPage() { telemetry_list[1] = {"Back", nullptr, nullptr}; }

void TelemetryPage::set_target_page(size_t index, IPage* page) { telemetry_list[0].target_page = page; }
void TelemetryPage::update_data(SensorData new_data) { data = new_data; }

void TelemetryPage::draw(OLED_display& display) {
    char temp[16];
    char hum[16];

    draw_thermometer_icon(display, 3, 2);
    display.draw_line(78, 0, 78, 11, 1);
    display.draw_str(15, 2, "Temp&Hum", 1);
    display.draw_str(84, 2, "14:02", true);

    display.draw_line(0, 11, 127, 11, 1);

    snprintf((char*)temp, sizeof(temp), "Temp: %.1f", data.temp);
    snprintf((char*)hum, sizeof(hum), "Hum: %.1f %%", data.humidity);

    display.draw_str(5, 20, temp, 1);
    display.draw_line(0, 35, 127, 35, 1);
    display.draw_str(5, 40, hum, 1);
}
void TelemetryPage::handle_input(UIEvent btn_ev, DisplayManager& display_mng) {
    if (btn_ev.btn_id == ButtonId::BACK) {
        display_mng.navigate_to(*telemetry_list[0].target_page);
    }
}
void draw_siren_icon(OLED_display& display, int16_t x, int16_t y) {
    display.draw_line(x + 1, y + 8, x + 7, y + 8, 1);

    display.draw_line(x + 2, y + 4, x + 2, y + 7, 1);
    display.draw_line(x + 6, y + 4, x + 6, y + 7, 1);
    display.draw_line(x + 3, y + 3, x + 5, y + 3, 1);

    display.draw_pixel(x + 4, y + 1, 1);
    display.draw_line(x, y + 2, x + 1, y + 3, 1);
    display.draw_line(x + 7, y + 3, x + 8, y + 2, 1);
}
AlarmPage::AlarmPage() { alarm_list[0] = {"Back", nullptr, nullptr}; }

void AlarmPage::set_target_page(size_t index, IPage* target) {
    if (index < options_cnt) {
        alarm_list[index].target_page = target;
    }
}

void AlarmPage::set_alarm_state(AlarmState state) { m_state = state; }

void AlarmPage::set_timer(uint8_t seconds) { m_timer_sec = seconds; }

AlarmState AlarmPage::get_state() { return m_state; }
void AlarmPage::set_contoler(AlarmControler* alarm_cntr_) { alarm_cntr = alarm_cntr_; }
void AlarmPage::draw(OLED_display& display) {
    m_state = alarm_cntr->get_state();
    draw_siren_icon(display, 3, 1);
    display.draw_line(78, 0, 78, 11, 1);
    display.draw_str(15, 2, "Alarm", 1);
    display.draw_str(84, 2, "14:02", true);
    display.draw_line(0, 11, 127, 11, 1);

    display.draw_str(5, 14, "STATE:", 1);

    const char* state_str = "DISARMED";
    switch (m_state) {
        case AlarmState::ARMED:
            state_str = "ARMING...";
            break;
        case AlarmState::TRIGGERED:
            state_str = "ALARM!";
            break;
        case AlarmState::DISARMED:
            state_str = "DISARMED";
        default:
            break;
    }
    display.draw_str(20, 24, state_str, 1);

    display.draw_line(0, 34, 127, 34, 1);

    display.draw_str(5, 37, "TIME TO LEAVE:", 1);

    char timer_buf[8];
    snprintf(timer_buf, sizeof(timer_buf), "%ds", m_timer_sec);
    display.draw_str(20, 49, timer_buf, 1);
}
void AlarmPage::handle_input(UIEvent btn_ev, DisplayManager& display_mng) {
    if (btn_ev.btn_id == ButtonId::BACK) {
        if (alarm_list[0].target_page != nullptr) {
            display_mng.navigate_to(*alarm_list[0].target_page);
        }
    }
}
void draw_gamepad_icon(OLED_display& display, int16_t x, int16_t y) {
    display.draw_line(x + 2, y, x + 8, y, 1);
    display.draw_line(x, y + 2, x, y + 6, 1);
    display.draw_line(x + 10, y + 2, x + 10, y + 6, 1);
    display.draw_line(x + 2, y + 7, x + 8, y + 7, 1);
    display.draw_pixel(x + 1, y + 1, 1);
    display.draw_pixel(x + 9, y + 1, 1);

    display.draw_pixel(x + 3, y + 3, 1);
    display.draw_pixel(x + 2, y + 4, 1);
    display.draw_pixel(x + 4, y + 4, 1);
    display.draw_pixel(x + 3, y + 5, 1);

    display.draw_pixel(x + 7, y + 3, 1);
    display.draw_pixel(x + 8, y + 4, 1);
    display.draw_pixel(x + 7, y + 5, 1);
}

GamePage::GamePage() {
    game_list[0] = {"Turtle Game", nullptr, nullptr};
    game_list[1] = {"Statistics", nullptr, nullptr};
    game_list[2] = {"Back", nullptr, nullptr};
}

void GamePage::set_target_page(size_t index, IPage* target) {
    if (index < options_cnt) {
        game_list[index].target_page = target;
    }
}

void GamePage::draw(OLED_display& display) {
    draw_gamepad_icon(display, 3, 2);
    display.draw_str(17, 2, "GAMES", 1);
    display.draw_line(78, 0, 78, 11, 1);
    display.draw_str(84, 2, "14:02", true);
    display.draw_line(0, 11, 127, 11, 1);

    const int16_t item_height = 22;
    const int16_t start_y = 15;

    for (size_t i = 0; i < options_cnt; ++i) {
        int16_t y = start_y + (i * item_height);
        bool is_selected = (static_cast<int8_t>(i) == selected_index);

        if (is_selected) {
            display.fill_rect(0, y, 128, item_height, 1);
            display.draw_str(10, y + 6, game_list[i].title, 0);
        } else {
            display.draw_str(10, y + 6, game_list[i].title, 1);
        }

        if (i < options_cnt - 1) {
            display.draw_line(0, y + item_height - 1, 127, y + item_height - 1, 1);
        }
    }
}

void GamePage::handle_input(UIEvent btn_ev, DisplayManager& display_mng) {
    switch (btn_ev.btn_id) {
        case ButtonId::DOWN:
            selected_index = (selected_index + 1) % options_cnt;
            break;

        case ButtonId::UP:
            selected_index = (selected_index == 0) ? (options_cnt - 1) : (selected_index - 1);
            break;

        case ButtonId::SELECT:
            if (game_list[selected_index].target_page != nullptr) {
                display_mng.navigate_to(*game_list[selected_index].target_page);
            }
            break;

        case ButtonId::BACK:
            if (game_list[2].target_page != nullptr) {
                display_mng.navigate_to(*game_list[2].target_page);
            }
            break;

        default:
            break;
    }
}
void draw_gear_icon(OLED_display& display, int16_t x, int16_t y) {
    display.draw_rect(x + 3, y + 3, 3, 3, 1);

    display.draw_pixel(x + 4, y + 1, 1);
    display.draw_pixel(x + 4, y + 7, 1);
    display.draw_pixel(x + 1, y + 4, 1);
    display.draw_pixel(x + 7, y + 4, 1);

    display.draw_pixel(x + 2, y + 2, 1);
    display.draw_pixel(x + 6, y + 2, 1);
    display.draw_pixel(x + 2, y + 6, 1);
    display.draw_pixel(x + 6, y + 6, 1);
}

SettingsPage::SettingsPage() {
    settings_list[0] = {"Brightness", nullptr, nullptr};
    settings_list[1] = {"Sound", nullptr, nullptr};
    settings_list[2] = {"WiFi Setup", nullptr, nullptr};
    settings_list[3] = {"Back", nullptr, nullptr};
}

void SettingsPage::set_target_page(size_t index, IPage* target) {
    if (index < options_cnt) {
        settings_list[index].target_page = target;
    }
}

void SettingsPage::draw(OLED_display& display) {
    draw_gear_icon(display, 3, 1);
    display.draw_str(15, 2, "SETTINGS", 1);
    display.draw_line(78, 0, 78, 11, 1);
    display.draw_str(84, 2, "14:02", true);
    display.draw_line(0, 11, 127, 11, 1);

    const int16_t item_height = 16;
    const int16_t start_y = 13;

    for (size_t i = 0; i < options_cnt; ++i) {
        int16_t y = start_y + (i * item_height);
        bool is_selected = (static_cast<int8_t>(i) == selected_index);

        if (is_selected) {
            display.fill_rect(0, y, 128, item_height, 1);
            display.draw_str(8, y + 3, settings_list[i].title, 0);
        } else {
            display.draw_str(8, y + 3, settings_list[i].title, 1);
        }

        if (i < options_cnt - 1) {
            display.draw_line(0, y + item_height - 1, 127, y + item_height - 1, 1);
        }
    }
}

void SettingsPage::handle_input(UIEvent btn_ev, DisplayManager& display_mng) {
    switch (btn_ev.btn_id) {
        case ButtonId::DOWN:
            selected_index = (selected_index + 1) % options_cnt;
            break;

        case ButtonId::UP:
            selected_index = (selected_index == 0) ? (options_cnt - 1) : (selected_index - 1);
            break;

        case ButtonId::SELECT:
            if (settings_list[selected_index].target_page != nullptr) {
                display_mng.navigate_to(*settings_list[selected_index].target_page);
            }
            break;

        case ButtonId::BACK:
            if (settings_list[3].target_page != nullptr) {
                display_mng.navigate_to(*settings_list[3].target_page);
            }
            break;

        default:
            break;
    }
}