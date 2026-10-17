#pragma once
#include <cstddef>
#include <cstdint>

class IPage;
typedef void (*MenuAction)();

enum class ButtonId { UP, DOWN, SELECT, BACK, NONE };
enum class AlarmEvents { MOTION_DETECTED, TEMPR_WARN, SYSTEM_BOOT };
enum class ScreanPages { MAIN_MENU, ALARM, TEMPR, GAME, GAME_STATS, SETTINGS };
enum class AlarmState { DISARMED, ARMED, TRIGGERED };
enum class AlarmCmd { ARM, DISARM, RESET_TRIGGER };
enum class UIEventType { BUTTON_PRESS, FORCE_NAVIGATE, REFRESH_SCREEN };

struct UIEvent {
    ButtonId btn_id;
    UIEventType type;
    IPage* target_page;
};
struct SensorData {
    float temp;
    float humidity;
    bool motion_detected;
};
struct ALarmMessage {
    AlarmEvents type;
    float data;
    uint32_t timecode;
};
struct SystemSettings {
    uint8_t exit_delay_sec;
    bool tg_notifications;
    bool siren_enabled;
};
struct MenuItem {
    const char* title;
    IPage* target_page;
    MenuAction action;
};