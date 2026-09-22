#pragma once
#include <string.h>

#include <cstddef>
#include <cstdint>

#include "SensorManager.hpp"
#include "Types.hpp"
class DisplayManager;
class OLED_display;
class AlarmControler;

class IPage {
public:
    virtual ~IPage() = default;
    virtual void draw(OLED_display& display) = 0;
    virtual void handle_input(UIEvent btn_ev, DisplayManager& display_mng) = 0;
};
class MenuPage : public IPage {
public:
    void draw(OLED_display& display) override;
    void handle_input(UIEvent btn_ev, DisplayManager& display_mng) override;
    static constexpr size_t options_cnt = 4;
    MenuItem menu_list[options_cnt];
    MenuPage();
    void set_target_page(size_t index, IPage* target);

private:
    int8_t selected_index = 0;
};

class TelemetryPage : public IPage {
public:
    void draw(OLED_display& display) override;
    void handle_input(UIEvent btn_ev, DisplayManager& display_mng) override;
    static constexpr int8_t options_cnt = 1;
    MenuItem telemetry_list[options_cnt];
    void set_target_page(size_t index, IPage* target);
    TelemetryPage();
    void update_data(SensorData data);

private:
    SensorData data;
};
class AlarmPage : public IPage {
public:
    void draw(OLED_display& display) override;
    void handle_input(UIEvent btn_ev, DisplayManager& display_mng) override;
    static constexpr int8_t options_cnt = 1;
    MenuItem alarm_list[options_cnt];
    void set_alarm_state(AlarmState state);
    void set_timer(uint8_t seconds);
    void set_target_page(size_t index, IPage* target);
    void set_contoler(AlarmControler* alarm_cntr_);
    AlarmState get_state();
    AlarmPage();

private:
    AlarmControler* alarm_cntr = nullptr;
    AlarmState m_state = AlarmState::DISARMED;
    uint8_t m_timer_sec = 10;
};
class GamePage : public IPage {
public:
    void draw(OLED_display& display) override;
    void handle_input(UIEvent btn_ev, DisplayManager& display_mng) override;
    void set_target_page(size_t index, IPage* target);
    GamePage();
    static constexpr int8_t options_cnt = 3;
    MenuItem game_list[options_cnt];

private:
    int8_t selected_index = 0;
};
class SettingsPage : public IPage {
public:
    void draw(OLED_display& display) override;
    void handle_input(UIEvent btn_ev, DisplayManager& display_mng) override;
    void set_target_page(size_t index, IPage* target);
    SettingsPage();
    static constexpr int8_t options_cnt = 4;
    MenuItem settings_list[options_cnt];

private:
    int8_t selected_index = 0;
};