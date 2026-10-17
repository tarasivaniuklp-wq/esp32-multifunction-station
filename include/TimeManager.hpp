#pragma once

#include <string>

#include "esp_log.h"
#include "esp_sntp.h"
#include "stdio.h"
#include "time.h"

class TimeManager {
private:
    static void notifier(struct timeval* tv);

public:
    static void init_sntp();
    static bool wait_for_sync(uint32_t timeout);
    TimeManager() = default;
    static std::string get_formated_time(const char* format = "%d.%m.%Y %H:%M:%S");
};